#include "earth.h"
#include "parseNMEA.h"
#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <algorithm>

namespace NMEA {

// constants to be used for checking sentence data within functions
std::list<std::string> const validLongitudes = {"E", "W"};
std::list<std::string> const validLatitudes = {"N", "S"}; 
std::list<std::string> static validStatuses = {"A", "V"};

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

    std::vector<std::string> values; // insert string data into a vector, excluding commas
    int start = 1;
    int counter = 1; // start and counter variables declared for extraction loop

    //while loop until checksum starts with *
    while (s[counter] != '*'){
	// Push values to vector from substring if they exist before a "*" or ","
        if((fieldStr[counter] == '*') || (fieldStr[counter] == ',')){
            std::string pushdata = fieldStr.substr(start, counter - start);
            values.push_back(pushdata);
            start = counter + 1;
        }
        counter++; // increment by 1 until extraction is finished

    }
    return {code, values}; // Return NMEA's string code along with vector data
}

GPS::Position positionFromSentenceData(SentenceData v)
{
	if (v.second.empty()) // check for empty field before proceeding
		throw std::invalid_argument("Empty field");

	if (v.first == "GLL") {
		if (v.second.size() != 5)
			throw std::invalid_argument("GLL invalid, incorrect size"); 

		std::stof(v.second[2]); // index 2 is longitude
		std::stof(v.second[0]); // index 0 is latitude - convert both to float from string

		if (!(std::find(validLongitudes.begin(), validLongitudes.end(), v.second[3]) != validLongitudes.end()))
        	throw std::invalid_argument("Directional symbol is not valid");
        char longitudeDirection = v.second[3][0];
		if (!(std::find(validLatitudes.begin(), validLatitudes.end(), v.second[1]) != validLatitudes.end()))
            throw std::invalid_argument("Directional symbol is not valid");
        char latitudeDirection = v.second[1][0];

		// format in correct NMEA data order - latitude value and direction, then latitude value and direction
        GPS::Position values(v.second[0], latitudeDirection, v.second[2], longitudeDirection);
        return values;
    }

	if (v.first == "RMC") {
		if (v.second.size() != 11)
			throw std::invalid_argument("RMC invalid, incorrect size");
		std::stof(v.second[0]); // value for timestamp
		std::stof(v.second[8]); // value for date
		std::stof(v.second[4]); // value for longitude
		std::stof(v.second[2]); // value for latitude

		if (!(std::find(validLongitudes.begin(), validLongitudes.end(), v.second[5]) != validLongitudes.end()))
            throw std::invalid_argument("Directional symbol is not valid");
        char longitudeDirection = v.second[5][0]; // direction for longitude value
		if (!(std::find(validLatitudes.begin(), validLatitudes.end(), v.second[3]) != validLatitudes.end()))
            throw std::invalid_argument("Directional symbol is not valid");
        char latitudeDirection = v.second[3][0]; // direction for latitude value

		// format in correct NMEA data order again
        GPS::Position values(v.second[2], latitudeDirection, v.second[4], longitudeDirection);
        return values;
    }

	if (v.first == "GGA") {
		if (v.second.size() != 14)
			throw std::invalid_argument("GGA invalid, incorrect size");
	    std::stof(v.second[0]); //value for timestamp
        std::stof(v.second[3]); //value for longitude
        std::stof(v.second[1]); //value for latitude 

		if (!(std::find(validLongitudes.begin(), validLongitudes.end(), v.second[4]) != validLongitudes.end()))
            throw std::invalid_argument("Directional symbol is not valid");
		char longitudeDirection = v.second[4][0]; //direction for longitude value
		if (!(std::find(validLatitudes.begin(), validLatitudes.end(), v.second[2]) != validLatitudes.end()))
            throw std::invalid_argument("Directional symbol is not valid");
		char latitudeDirection = v.second[2][0]; //direction for latitude value

		//format in correct NMEA sentence data order
		GPS::Position values(v.second[1], latitudeDirection, v.second[3], longitudeDirection, v.second[8]);
        return values;
    }
	throw std::invalid_argument("Word format is not supported!");	
}

Route routeFromLog(std::istream &v)
{
	//Route routeTaken;
	//std::string sentence
	// Stub definition, needs implementing
    return {};

}


}

