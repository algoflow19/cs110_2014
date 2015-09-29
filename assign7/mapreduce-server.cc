/**
 * File: mapreduce-server.cc
 * -------------------------
 * Presents the implementation of the MapReduceServer class.
 */

#include "mapreduce-server.h"
#include <iostream>              // for cout
#include <getopt.h>              // for getopt_long
#include <sstream>               // for ostringstream
#include <functional>            // for hash<string>
#include <climits>               // for USHRT_MAX
#include <cstring>               // for memset
#include <vector>                // for vector
#include <set>                   // for set
#include <algorithm>             // for random_shuffle
#include <fstream>               // for ifstream
#include <sys/types.h>           // for struct stat
#include <sys/stat.h>            // for stat
#include <unistd.h>              // for stat
#include <netdb.h>               // for gethostbyname, struct hostent *
#include <sys/socket.h>          // for socket, bind, accept, listen, etc.
#include <arpa/inet.h>           // for htonl, htons, etc.  
#include <dirent.h>              // for DIR, dirent
#include <socket++/sockstream.h>
#include "mr-nodes.h"            // for getting list of nodes in myth cluster that actually work!
#include "mr-messages.h"         // for messages between client and server
#include "mr-env.h"              // for getUser, getHost, etc.
#include "string-utils.h"        // for trim
#include "server-socket.h"       // for createServerSocket, kServerSocketFailure
#include "ostreamlock.h"         // for oslock, osunlock
#include "mapreduce-server-exception.h"
using namespace std;

/**
 * Constructor: MapReduceServer
 * ----------------------------
 * Configures the server using the well-formed contents of the configuration file supplied
 * on the command line, and then launches the server to get ready for the flash mob
 * of worker requests to map and reduce data input files.
 */
static const size_t kNumWorkerThreads = 16;
MapReduceServer::MapReduceServer(int argc, char *argv[]) throw (MapReduceServerException) 
  : serverPort(computeDefaultPortForUser()), serverIsRunning(false) {
  ensureEnvironmentVariablesPresent();
  parseArgumentList(argc, argv);  
  startServer();
  logServerConfiguration(cout);
}

/**
 * Method: run
 * -----------
 * Presents the high-level scripts of what the server must accomplish in order to ensure that
 * an entire MapReduce job executes to completion.  See the documentation for each of the
 * methods called by run to gain a better sense of what's accomplished here.
 */
void MapReduceServer::run() throw() {
  buildIPAddressMap();
  stageInputFiles(mapInputPath);
  spawnMappers();
  cout << "Mapping of all input chunks now complete." << endl;
  // Each of the following five lines should be reintroduced as you implement Tasks 3 and 4.
  // groupByKey();
  // cout << "Reduce input files have been generated, sorted, and grouped by key." << endl << endl;
  // stageInputFiles(reduceInputPath);
  // spawnReducers();
  // cout << "Reduction of all grouped, intermediate chunks now complete." << endl;
}

/**
 * Method: ensureEnvironmentVariablesPresent
 * -------------------------------------------
 * Confirms that we have actual values for the current user, the host, and the
 * current working directory.  If one of them is missing, then an instance
 * of MapReduceServerException (surrounding a relevant what() message) is thrown.
 */
static const struct {
  const char *(*envfn)();
  const char *value;
} kDependencies[] = {
  {getUser, "logged in user"},
  {getHost, "host machine"},
  {getCurrentWorkingDirectory, "current working directory"},
};
void MapReduceServer::ensureEnvironmentVariablesPresent() throw (MapReduceServerException) {
  for (const auto& d: kDependencies) {
    if (d.envfn() == NULL) {
      ostringstream oss;
      oss << "The " << d.value << " could not be determined from the set of environment variables.";
      throw MapReduceServerException(oss.str());      
    }
  }
}

/**
 * Method: parseArgumentList
 * -------------------------
 * Self-explanatory.  The --config/-c flag is required, the --port/-p flag.
 */
static const string kUsageString = "./mr --config <config-file> [--port <port>]";
static const unsigned short kDefaultServerPort = 12345;
void MapReduceServer::parseArgumentList(int argc, char **argv) throw (MapReduceServerException) {
  struct option options[] = {
    {"port", required_argument, NULL, 'p'},
    {"config", required_argument, NULL, 'c'},
    {NULL, 0, NULL, 0},
  };
  
  ostringstream oss;
  string configFilename;
  while (true) {
    int ch = getopt_long(argc, argv, "p:c:", options, NULL);
    if (ch == -1) break;
    switch (ch) {
    case 'p':
      serverPort = extractPortNumber(optarg);
      break;
    case 'c':
      configFilename = optarg;
      break;
    default:
      oss << "Unrecognized or improperly supplied flag." << endl;
      oss << kUsageString;
      throw MapReduceServerException(oss.str());
    }
  }

  argc -= optind;
  if (argc > 0) {
    oss << "Too many arguments." << endl;
    oss << kUsageString;
    throw MapReduceServerException(oss.str());
  }

  if (configFilename.empty()) {
    oss << "You must supply a configuration file as an argument." << endl;
    oss << kUsageString;
    throw MapReduceServerException(oss.str());
  }

  initializeFromConfigFile(configFilename);
}

/**
 * Method: computeDefaultPortForUser
 * ---------------------------------
 * Uses the username and hostname to compute a default server
 * port number (to be used in the event one isn't supplied
 * on the command line).  Care was taken to make sure the
 * default port computed here is different than that used
 * for the http-proxy assignment, just so both "mr" and "http-proxy"
 * can easily be run at the same time without providing override port
 * numbers.
 */
static const unsigned short kLowestOpenPortNumber = 1024;
unsigned short MapReduceServer::computeDefaultPortForUser() {
  string user = getUser();
  string host = getHost();
  size_t hashValue = hash<string>()(user + "@" + host); // ensure different than http-proxy default port
  return hashValue % (USHRT_MAX - kLowestOpenPortNumber) + kLowestOpenPortNumber;
}

/**
 * Method: extractPortNumber
 * -------------------------
 * If the user wishes to supply a port number on the command line (probably because their
 * default port number is in use for some reason), then extractPortNumber is called.
 * In a nutshell, it relies on strtol to convert a string to a number, and it confirms that
 * the string was purely numeric and that the number is a legitimate port number.
 */
unsigned short MapReduceServer::extractPortNumber(const char *portArgument) throw (MapReduceServerException) {
  char *endptr;
  long rawPort = strtol(portArgument, &endptr, 0);
  if (*endptr != '\0') {
    ostringstream oss;
    oss << "The supplied port number of \"" << portArgument << "\" is malformed.";
    throw MapReduceServerException(oss.str());
  }
  
  if (rawPort < 1 || rawPort > USHRT_MAX) {
    ostringstream oss;
    oss << "Port number must be between 1 and " << USHRT_MAX << ".";
    throw MapReduceServerException(oss.str());
  }
  
  return (unsigned short) rawPort;
}

/**
 * Method: initializeFromConfigFile
 * --------------------------------
 * Opens the supplied configuation file, confirms that it exists, confirms
 * that the configuation file is well-formed and supplies all of the necessary
 * information, then updates the server object with the data found within the
 * file, confirms all directories identified within the configuration file (be they absolute or
 * relative to the executable directory) exist.  Oodles of error checking is done here to minimize
 * the possibility that the server crashes in some more oblique way later on.
 */
static const string kConfigFileKeys[] = {
  "mapper", "reducer", "num-mappers", "num-reducers",
  "input-path", "executable-path", "map-output-path", 
  "reduce-input-path", "output-path"
};
static const size_t kNumConfigFileKeys = sizeof(kConfigFileKeys)/sizeof(kConfigFileKeys[0]);
void MapReduceServer::initializeFromConfigFile(const string& configFileName) throw (MapReduceServerException) {
  ifstream infile(configFileName);
  if (!infile) {
    ostringstream oss;
    oss << "Configuration file named \"" << configFileName << "\" could not be opened.";
    throw MapReduceServerException(oss.str());
  }

  set<string> requiredKeys(kConfigFileKeys, kConfigFileKeys + kNumConfigFileKeys);
  set<string> suppliedKeys;
  while (true) {
    string key;
    getline(infile, key, /* stopchar = */ ' ');
    if (infile.fail()) break;
    key = trim(key);
    if (key.empty()) continue;
    if (requiredKeys.find(key) == requiredKeys.cend()) {
      ostringstream oss;
      oss << "Configuration file key of \"" << key << "\" not recognized.";
      throw MapReduceServerException(oss.str());
    }
    if (suppliedKeys.find(key) != suppliedKeys.cend()) {
      ostringstream oss;
      oss << "Configuration file key of \"" << key << "\" supplied multiple times.";
      throw MapReduceServerException(oss.str());
    }
    suppliedKeys.insert(key);
    string value;
    getline(infile, value); // read rest of line
    applyToServer(key, trim(value));
  }
  
  if (suppliedKeys != requiredKeys) {
    ostringstream oss;
    oss << "One or more required keys missing from configuration file." << endl;
    throw MapReduceServerException(oss.str());
  }
}

/**
 * Method: applyToServer
 * ---------------------
 * Dispatches on each of the various key values and assigns the corresponding
 * server data member to the supplied value.  Relative directories are canonicalized
 * to be absolute directories, and the number of mappers and number of reducers are
 * forced to be a small, positive number.
 */
static const size_t kMinWorkers = 1;
static const size_t kMaxWorkers = kNumWorkerThreads;
void MapReduceServer::applyToServer(const string& key, const string& value) throw (MapReduceServerException) {
  if (key == "mapper") {
    mapperExecutable = value;
  } else if (key == "reducer") {
    reducerExecutable = value;
  } else if (key == "num-mappers") {
    numMappers = parseNumberInRange(key, value, kMinWorkers, kMaxWorkers);
  } else if (key == "num-reducers") {
    numReducers = parseNumberInRange(key, value, kMinWorkers, kMaxWorkers);
  } else if (key == "input-path") {
    mapInputPath = ensureDirectoryExists(key, value);
  } else if (key == "executable-path") {
    executablePath = ensureDirectoryExists(key, value);
  } else if (key == "map-output-path") {
    mapOutputPath = ensureDirectoryExists(key, value);
  } else if (key == "reduce-input-path") {
    reduceInputPath = ensureDirectoryExists(key, value);
  } else if (key == "output-path") {
    reduceOutputPath = ensureDirectoryExists(key, value);
  }
}

/**
 * Method: ensureDirectoryExists
 * -----------------------------
 * Ensures that the supplied path exists and is visible to the user.
 * Returns the absolute version of the path, even if it was expressed as relative,
 * because these paths needs to be passed across the network between the server and the
 * workers.
 */
string MapReduceServer::ensureDirectoryExists(const string& key, const string& path) throw (MapReduceServerException) {
  string absolutePath = path;
  if (!path.empty() && path[0] != '/') {
    ostringstream oss;
    oss << getCurrentWorkingDirectory() << "/" << path;
    absolutePath = oss.str();
  }

  struct stat st;
  int status = stat(path.c_str(), &st);
  if (status == -1) {
    ostringstream oss;
    oss << "The configuration file's \"" << key << "\" entry identifies a path" << endl
        << "of \"" << path << "\" that either doesn't exist" << endl
        << "or is unreadable.";
    throw MapReduceServerException(oss.str());
  }

  if (!(st.st_mode & S_IFDIR)) {
    ostringstream oss;
    oss << "The configuration file's \"" << key << "\" entry identifies a path" << endl
        << "of \"" << path << "\" that isn't a directory.";
    throw MapReduceServerException(oss.str());
  }

  return absolutePath;
}

/**
 * Method: parseNumberInRange
 * --------------------------
 * Confirms that the supplied value is indeed a number with the range [low, high].
 */
size_t MapReduceServer::parseNumberInRange(const std::string& key, const std::string& value, 
                                           size_t low, size_t high) throw (MapReduceServerException) {
  char *endptr;
  long number = strtol(value.c_str(), &endptr, 0);
  if (*endptr != '\0') {
    ostringstream oss;
    oss << "The configuration file's \"" << key << "\" entry is bound to a value" << endl
        << "of \"" << value << "\", but that value needs to be a positive integer.";
    throw MapReduceServerException(oss.str());
  }
  
  size_t positiveNumber = number;
  if (number < 0 || positiveNumber < low || positiveNumber > high) {
    ostringstream oss;
    oss << "The configuration file's \"" << key << "\" entry is bound to the" << endl
        << "number " << positiveNumber << ", but that number needs be within the range [" << low << ", " << high << "].";
    throw MapReduceServerException(oss.str());
  }

  return positiveNumber;
}

/**
 * Method: logServerConfiguration
 * ------------------------------
 * Publishes the state of the server object (assuming it's been fully
 * initialized) just so we can sanity check all of the values that will certainly
 * influence execution.
 */
void MapReduceServer::logServerConfiguration(ostream& os) throw() {
  os << "Mapper executable: " << mapperExecutable << endl;
  os << "Reducer executable: " << reducerExecutable << endl;
  os << "Number of Mapping Workers: " << numMappers << endl;
  os << "Number of Reducing Workers: " << numReducers << endl;
  os << "Executable Path: " << executablePath << endl;
  os << "Map Input Path: " << mapInputPath << endl;
  os << "Map Output Path: " << mapOutputPath << endl;
  os << "Reduce Input Path: " << reduceInputPath << endl;
  os << "Reduce Output Path: " << reduceOutputPath << endl;
  os << "Server running on port " << serverPort << endl;
  os << endl;
}

/**
 * Method: buildIPAddressMap
 * -------------------------
 * Compiles a map of IP addresses (e.g. "171.64.64.123") to hostname (e.g. "myth21.stanford.edu").
 * This is done so that logging messages can print hostnames instead of IP addresses, even though
 * IP addresses are more readily surfaced by all of the socket API functions.
 */
static const vector<string> kMapReduceNodes = loadMapReduceNodes();
void MapReduceServer::buildIPAddressMap() throw() {
  string serverHost = getenv("HOST");
  ipAddressMap["127.0.0.1"] = serverHost + ".stanford.edu";
  for (const string& node: kMapReduceNodes) {
    struct hostent *he = gethostbyname(node.c_str());
    if (he == NULL) {
      cerr << node << ".stanford.edu is unreachable." << endl;
      continue;
    }
    for (size_t i = 0; he->h_addr_list[i] != NULL; i++) {
      string ipAddress = inet_ntoa(*(struct in_addr *)(he->h_addr_list[0]));
      ipAddressMap[ipAddress] = node + ".stanford.edu";
    }
  }
}

/**
 * Method: stageInputFiles
 * -----------------------
 * Examines the names directory (which, in practice, should be this->mapInputPath when staging
 * for the mapping phase, and this->reduceInputPath when staging for the reduction phase),
 * and populates the unprocessedChunks queue with the names of all of the directory
 * entries.
 */
void MapReduceServer::stageInputFiles(const std::string& inputDirectory) throw() {
  DIR *dir = opendir(inputDirectory.c_str());
  cout << "Identifying chunks in \"" << inputDirectory << "\" to be processed." << endl;
  while (true) {
    struct dirent *ent = readdir(dir);
    if (ent == NULL) break;
    if (ent->d_name[0] == '.') continue; // ".", "..", or some hidden file should be ignored
    string chunk(inputDirectory);
    chunk += "/";
    chunk += ent->d_name;
    cout << "+  Adding chunk to list of chunks yet to be processed: \"" << chunk << "\"." << endl;
    unprocessedChunks.push_back(chunk);
  }
  closedir (dir);
  cout << endl;
}

/**
 * Method: startServer
 * -------------------
 * Creates a server socket to listen in on the (possibly command-line supplied)
 * server port, and then launches the server itself in a separate thread.
 * (The server needs to run off the main thread, because the main thread needs to move on
 * to stage input files, spawn workers to apply mappers to those input files, wait
 * until all input files have been processed, run groupByKey, and so forth.
 */
static const int kMaxBacklog = 128;
void MapReduceServer::startServer() throw (MapReduceServerException) {
  serverSocket = createServerSocket(serverPort, kMaxBacklog);
  if (serverSocket == kServerSocketFailure) {
    ostringstream oss;
    oss << "Port " << serverPort << " is already in use, so server could not be launched.";
    throw MapReduceServerException(oss.str());
  }

  serverThread = thread([this] { orchestrateWorkers(); });
}

/**
 * Method: orchstrateWorkers
 * -------------------------
 * Implements the canonical server, which loops interminably, handling incoming
 * network requests which, in this case, will be requests from the farm of previously
 * spawned workers.
 *
 * (The initial version of the code as it's been given to you handles all requests
 * sequentially, within the server thread.  That will need to change as per the assignment
 * requirements outlined by Task 2).
 */
void MapReduceServer::orchestrateWorkers() throw () {
  serverIsRunning = true;
  while (true) {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);
    memset(&clientAddress, 0, clientAddressSize);
    int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddress, &clientAddressSize);
    if (!serverIsRunning) { close(clientSocket); close(serverSocket); break; }
    string clientIPAddress(inet_ntoa(clientAddress.sin_addr));
    cout << oslock << "Received a connection request from " 
         << ipAddressMap[clientIPAddress] << "." << endl << osunlock;
    handleRequest(clientSocket, clientIPAddress);
  }
}

/**
 * Method: handleRequest
 * ---------------------
 * Manages the incoming message from some worker running on the supplied IP address and,
 * in some cases, responds to the same worker with some form of an acknowledgement that the message was
 * received.
 *
 * Note that this implementation, as it's given to you, is incomplete and needs to be updated
 * just slightly as part of Task 1.
 */
void MapReduceServer::handleRequest(int clientSocket, const string& clientIPAddress) throw() {
  cout << oslock << "Incoming communication from " << ipAddressMap[clientIPAddress]
       << " on descriptor " << clientSocket << "." << endl << osunlock;
  sockbuf sb(clientSocket);
  iostream ss(&sb);
  MRMessage message;
  string payload;
  try {
    receiveMessage(ss, message, payload);
  } catch (exception& e) {
    cout << oslock << "Spurious connection received from " << ipAddressMap[clientIPAddress]
         << " on descriptor " << clientSocket << "." << endl << osunlock;
    return;
  }
  
  if (message == kWorkerReady) {
    string chunkName;
    bool success = surfaceNextChunk(chunkName);
    if (success) {
      cout << oslock << "Instructing worker at " << ipAddressMap[clientIPAddress] << " to process "
           << "chunk named \"" << chunkName << "\"." << endl << osunlock;
      sendJobStart(ss, chunkName);
    } else { // no more chunks to process, so kill the worker
      cout << oslock << "Informing worker at " << ipAddressMap[clientIPAddress] << " that all "
           << "chunks have been processed." << endl << osunlock;
      sendServerDone(ss); // informs worker to exit, server side system call returns, surrounding thread exits 
    }
  } else if (message == kJobSucceeded) {
    string chunkName = trim(payload);
    markChunkAsProcessed(clientIPAddress, chunkName);
  } else if (message == kJobInfo) {
    string workerInfo = trim(payload);
    cout << oslock << workerInfo << endl << osunlock;
  } else {
    cout << oslock << "Ignoring unrecognized message type of \"" 
         << message << "\"." << endl << osunlock;
  }

  cout << oslock << "Conversation with " << ipAddressMap[clientIPAddress] 
       << " complete." << endl << osunlock;
}

/**
 * Method: surfaceNextChunk
 * ------------------------
 * Surfaces the next input file that should be processed by some worker eager to do some work.
 * The method returns false if there are no more files to be processed, but it returns
 * true if there was at least one (and it surfaces the name of that input file by
 * populated the space referenced by chunkName with it).
 */
bool MapReduceServer::surfaceNextChunk(std::string& chunkName) throw() {
  if (unprocessedChunks.empty()) return false;
  chunkName = unprocessedChunks.front();
  unprocessedChunks.pop_front();
  return true;
}

/**
 * Method: markChunkAsProcessed
 * ----------------------------
 * Acknowledges that the worker spinning on the remote machine (as identified by
 * the supplied IP address) managed to fully process the named file.
 */
void MapReduceServer::markChunkAsProcessed(const std::string& clientIPAddress, 
                                           const std::string& chunkName) throw() {
  cout << oslock << "Chunk named \"" << chunkName << "\" " 
       << "fully processed by worker at " << ipAddressMap[clientIPAddress] << "." 
       << endl << osunlock;
}

/**
 * Method: rescheduleChunk
 * -----------------------
 * Acknowledges that a worker spinning on some remote machine (identified via its IP address) failed 
 * to fully process the supplied input file (probably because a mapper or reducer executable
 * returned a non-zero exit status), and rescheduled to same input file to be processed
 * later on.  Rather than reissuing the job for the same input file to the same machine right away,
 * we instead append the input file to the end of the queue of unprocessed files and schedule
 * it later, when it bubbles to the front.
 */
void MapReduceServer::rescheduleChunk(const std::string& clientIPAddress, 
                                      const std::string& chunkName) throw() {
  unprocessedChunks.push_back(chunkName);
  cout << oslock << "Chunk named \"" << chunkName << "\" " 
       << "not properly processed by worker at " << ipAddressMap[clientIPAddress] << ", so rescheduling." 
       << endl << osunlock;
}

/**
 * Method: spawnMappers
 * --------------------
 * Launches this->numMappers workers, one per remote machine, across this->numMappers
 * remote hosts.  
 * 
 * (The initial code only spawns one worker, regardless of the
 * value of this->numMappers, but Task 2 of your Assignment 7 handout outlines
 * what changes need to be made to upgrade it to spawn the full set of mapper workers
 * so that map jobs can be done in parallel across multiple machines instead of
 * in sequence on just one).
 */
void MapReduceServer::spawnMappers() throw() {
  vector<string> mapperNodes(kMapReduceNodes);
  random_shuffle(mapperNodes.begin(), mapperNodes.end());
  string command = buildCommand(mapperNodes[0], executablePath, mapperExecutable, mapOutputPath);
  spawnWorker(mapperNodes[0], command);
}

/**
 * Method: spawnReducers
 * ---------------------
 * Launches this->numReducers workers, one per remote host, across 
 * this->numReducers remote hosts.  Modeled after the implementation of 
 * spawnMappers, which basically does the same thing, except that a
 * different executable (a reducer instead of a mapper) and a different
 * output path are supplied as part of the command to be invoked via
 * ssh on different machines.
 */
void MapReduceServer::spawnReducers() throw() {
  // you should implement this to emulate what spawnMappers does, except that
  // the reducer executable should be used, and output files should be
  // published to a different directory.  See the Task 4 description 
  // in the handout for more information.
}

/**
 * Method: buildCommand
 * --------------------
 * Constructs the command needed to invoke a worker on a remote machine using
 * the system (see "man system") function.  See spawnWorker below for even more information.
 */
static const string kWorkerExecutable = "mrw";
string MapReduceServer::buildCommand(const string& workerHost, const string& executablePath, 
                                     const string& executable, const string& outputPath) throw() {
  string userName = getUser();
  string serverHost = getHost();
  ostringstream oss;
  oss << "ssh -o ConnectTimeout=5 " << userName << "@" << workerHost
      << " '" << executablePath << "/" + kWorkerExecutable
      << " " << serverHost
      << " " << serverPort
      << " " << executablePath
      << " " << executable
      << " " << outputPath << "'";
  return oss.str();
}

/**
 * Method: spawnWorker
 * -------------------
 * Assumes the incoming command is of the form 'ssh poohbear@myth8.stanford.edu '/usr/class/cs110/<other-directories>/mrw <arg1> ... <arg5>',
 * which is precisely the command that can be used to launch a worker on a remote machine (in this example, myth8).
 * The remote command is invoked vi the system function, which blocked until the remotely invoked command--in this case, a command
 * to run a worker--terminates.
 */

void MapReduceServer::spawnWorker(const string& node, const string& command) throw() {
  cout << oslock << "Spawning worker on " << node << " with ssh command: " << endl 
       << "\t\"" << command << "\"" << endl << osunlock;
  int status = system(command.c_str());
  if (status != -1) status = WEXITSTATUS(status);
  cout << oslock << "Remote ssh command on " << node << " executed and returned a status " 
       << status << "." << endl << osunlock;
}

/**
 * Method: groupByKey
 * ------------------
 * Iterates over each and every file output by the mapping phase of the job, distributing
 * each line to one of this->numReducers files.  Those files are then sorted and grouped
 * by key, where grouping-by-key means coalescing consecutives key/value pair lines into
 * a single key/vector-of-values line, e.g. the file:
 *
 *   ivan evan
 *   ivan wanda
 *   jon alice
 *   jon jerry
 *   jon julie
 *   jon scott
 *   jon zoe
 *   kathy john
 *   margaret bobby
 *   margaret roberta
 *
 * would be compressed to be this:
 * 
 *   ivan evan wanda
 *   jone alice jerry julie scott zoe
 *   kathy john
 *   margaret bobby roberta
 * 
 * The number of reducers dictates how many grouped-by-key files there are, and
 * the hash of the key (as generated by a hash<string> object) modulo the number of
 * reducers dictates which file the key should be marshaled to.
 */
void MapReduceServer::groupByKey() throw() {
  // you should implement this function as outlined in the above documentation and
  // in the Task 3 description of the handout.
}

/**
 * Destructor: ~MapReduceServer
 * ----------------------------
 * Instructs the server to shut itself down, waits for that
 * to happen, and then destroys all of the directly embedded objects
 * that contribute to the MapReduce system.
 */
MapReduceServer::~MapReduceServer() throw() {
  // assumption is that thread pool is empty and that server is asleep in an accept call
  bringDownServer();
  serverThread.join();
  cout << "Server has shut down." << endl;
}

/**
 * Method: bringDownServer
 * -----------------------
 * Pings the serverThread, which is assumed to be blocked within an accept call,
 * so that it wakes up and shuts itself down.  If you look at the implementation of
 * orchestrateWorkers above, you'll see the first thing it does is check to see
 * it the server is no longer running, and if so, closes down all of its server-side
 * resources and exits.
 */
void MapReduceServer::bringDownServer() throw() {
  serverIsRunning = false;
  shutdown(serverSocket, SHUT_RD);
}
