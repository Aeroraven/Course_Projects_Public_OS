#include "FSCore.h"
#include <iostream>
# pragma execution_character_set("utf-8")
using namespace std;
void FSCore::fsMemcpy(db* src, db* dst, dd size) {
	while (size--) {
		*dst++ = *src++;
	}
}
void FSCore::fsMemset(db* dst, db val, dd size) {
	while (size--)
		*dst++ = val;
}
void FSCore::fsSplit(const string& s, vector<string>& tokens, const string& delimiters){
	string::size_type lastPos = s.find_first_not_of(delimiters, 0);
	string::size_type pos = s.find_first_of(delimiters, lastPos);
	while (string::npos != pos || string::npos != lastPos) {
		tokens.push_back(s.substr(lastPos, pos - lastPos));
		lastPos = s.find_first_not_of(delimiters, pos);
		pos = s.find_first_of(delimiters, lastPos);
	}
}