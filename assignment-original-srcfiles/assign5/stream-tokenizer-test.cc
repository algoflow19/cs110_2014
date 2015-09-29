/**
 * File: stream-tokenizer-test.cc
 * ------------------------------
 * Very simple test harness to do a sanity 
 * check on our StreamTokenizer class.
 */

#include <string>
#include <sstream>
#include <iostream>
#include "stream-tokenizer.h"
using namespace std;

static void testStreamTokenizer(const string& text, const string& delimiters, bool skipDelimiters) {
  istringstream iss(text);
  StreamTokenizer st(iss, delimiters, skipDelimiters);
  while (st.hasMoreTokens()) {
    string token = st.nextToken();
    cout << "Token: \"" << token << "\"" << endl;
  }
  cout << endl;
}

int main(int argc, const char *argv[]) {
  testStreamTokenizer(/* text = */ "Hey there my friend. How are you doing? ",
		      /* delimiters = */ " \t\n\r\b!@#$%^&*()_-+=~`{[}]|\\\"':;<,>.?/",
		      /* skipDelimiters = */ true);
  testStreamTokenizer(/* text = */ "Hey there my friend. How are you doing? ",
		      /* delimiters = */ " \t\n\r\b!@#$%^&*()_-+=~`{[}]|\\\"':;<,>.?/",
		      /* skipDelimiters = */ false);
  testStreamTokenizer(/* text = */ "Hey there my ü friend.   ",
		      /* delimiters = */ " .ü",
		      /* skipDelimiters = */ true);
  testStreamTokenizer(/* text = */ "Hey there my ü friend.   ",
		      /* delimiters = */ " .ü",
		      /* skipDelimiters = */ false);
  return 0;
}
