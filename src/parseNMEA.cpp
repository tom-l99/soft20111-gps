#include "earth.h"
#include "parseNMEA.h"
#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <algorithm>

namespace NMEA {

bool isWellFormedSentence(std::string s)
{
    std::string sentence = s; 
    unsigned int sentenceLength = sentence.length();

    std::string prefix = "$GP";
    if (!(sentence.rfind(prefix, 0) == 0)) {
        return false; //ensuring invalid sentence prefix not present
    }

    std::string sentenceFormat = sentence.substr(3, 3); // substring of 3 characters after sentence prefix
    if (!std::regex_match(sentenceFormat, std::regex("[A-Z]{3,3}"))) {
        //regular expression requiring characters to be English alphabet in capitals, with a length of 3
        return false;
    }

    std::string checkSum = sentence.substr((sentenceLength-3), 3);
    if (!std::regex_match(checkSum, std::regex("[*][0-9A-Fa-f][0-9A-Fa-f]"))) {
	// regular expression that requires substring to have a *, followed by two valid hex characters (0-9 or A-F in upper or lower)
        return false;
    }

    unsigned long asterisk = sentence.find_last_of("*");
    unsigned long comma = sentence.find_first_of(","); 
    // find position of last "*" character and position of first "," character

    for (; comma < asterisk; comma++) {
        // for loop to ensure no reserved characters exist between the two positions
        if (sentence[comma]=='*' || sentence[comma]=='$')
            return false; // function returns false if found
    }

    return true;
}

bool hasValidChecksum(std::string s)
{
	
    std::string sentence = s; 

    unsigned int sentenceLength = sentence.length();
    unsigned int lastChar = sentence.find("*");
    unsigned int value = sentence.find("$") + 1;

    int64_t totalcheckSum = 0;
    for (; value < lastChar; value++)
		// loop through string and XOR sentence to calculate the checkSum 
        totalcheckSum = totalcheckSum ^ sentence[value];

    std::stringstream ss; 
    ss << std::hex << totalcheckSum; 
    std::string hexChecksum = ss.str(); 
	// format stringstream of checksum into hex

    std::string sentenceChecksum = sentence.substr((sentenceLength - 2), 2);
	// Remove "$" and checksum value from the string
    std::transform(sentenceChecksum.begin(), sentenceChecksum.end(), sentenceChecksum.begin(), ::toupper);
    std::transform(hexChecksum.begin(), hexChecksum.end(), hexChecksum.begin(), ::toupper);

    if (!(hexChecksum.compare(sentenceChecksum) == 0))
        return false;

    return true;
}

SentenceData extractSentenceData(std::string s)
{
    std::string code = s.substr(3,3); // substring for NMEA code
    std::string fieldStr = s.substr(6); // substring after NMEA code

    //unsigned int fieldLength = fields.length();


    std::vector<std::string> data; // insert string data into a vector, excluding commas
    int start = 1;
    int counter = 1; // start and counter variables declared for extraction loop

    //while loop until checksum starts with *
    while (s[counter] != '*'){
	// Push values to vector from substring if they exist before a "*" or ","
        if((fieldStr[counter] == '*') || (fieldStr[counter] == ',')){
            std::string pushdata = fieldStr.substr(start, counter - start);
            data.push_back(pushdata);
            start = counter + 1;
        }
        counter++; // increment by 1 until extraction is finished

    }
    return {code, data}; // Return NMEA's string code along with vector data
}

GPS::Position positionFromSentenceData(SentenceData)
{
    // Stub definition, needs implementing
    return GPS::Earth::NorthPole;
}

Route routeFromLog(std::istream &)
{
    // Stub definition, needs implementing
    return {};
}


}

