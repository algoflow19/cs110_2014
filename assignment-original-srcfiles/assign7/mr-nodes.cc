/**
 * File: mr-nodes.cc
 * -----------------
 * Presents the (very straightforward) implementation of
 * loadMapReduceNodes.  In an ideal world this would actually
 * build a list of myth hostnames that were reachable and ssh'able,
 * but right now it just loads the list of hostnames from a file. #weak
 */

#include "mr-nodes.h"
#include "string-utils.h"
#include <iostream>
#include <fstream>
using namespace std;

vector<string> loadMapReduceNodes() {
  const string kMapReduceNodesInputFile = "/usr/class/cs110/samples/assign7-config/nodes.txt";
  ifstream instream(kMapReduceNodesInputFile);
  if (!instream) {
    cerr << "Failed to load list of available mr nodes from \"" 
         << kMapReduceNodesInputFile << "\"." << endl;
    cerr << "Aborting..." << endl;
    exit(1);
  }

  vector<string> nodes;
  while (true) {
    string node;
    getline(instream, node);
    if (instream.fail()) return nodes;
    nodes.push_back(trim(node));
  }
}


