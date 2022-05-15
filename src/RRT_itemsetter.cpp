//v0.1
//made by Cipnit with help from Nahnegnal
//code requires C++11

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#include <windows.h>
#include <stdlib.h>
using namespace std;


bool SetOffsets(ifstream* ROMOffsets);

int ConvertListToBin(string listfilename);
int ConvertListToInsertInROM(fstream* RRTROM, string listfilename, unsigned char itemsetID);
int CreateItemSetBinaryData(ifstream* ItemSetList, vector<unsigned char>* ItemSetBin, string& ErrorMessages);

int ConvertBinToList(string binfilename, ifstream* DefaultItemSetList);
int ConvertBinFromROMToList(fstream* RRTROM, ifstream* DefaultItemSetList, unsigned char itemsetID);
int ExtractItemSetBinaryData(fstream* ItemSetBin, ofstream* OutputItemSetList, ifstream* DefaultItemSetList);

unsigned char GetDungeonID(fstream* RRTROM, string& arguing);
int GetDungeonItemSets(fstream* RRTROM, unsigned char dungID, string dungName);
unsigned int seekROMToItemSetAddress(fstream* RRTROM, unsigned char itemsetID);
int getBinItemSetSize(fstream* ItemSetBin);
int updateCurrentCategory(int currententry);


int _ItemSetPointerTable;
int _DungeonFloorDataPointerTable;
int _DungeonSizes;
int _DungeonNamesPointerTable;
int _HighestDungeonID;
#define _MaxItemSetEntries 252 //NOTE: If you want to change the amount of items and categories, you'll have
#define _MaxCategories 12	   //to change a lot more than just these definitions.



int main(int argc, char *argv[])
{
	string arguments[4];
	fstream RRTROM;
	ifstream DefaultItemSetList("DEFAULT_ITEMSET_LIST.txt");
	ifstream ROMOffsets("offsets.txt");
	cout << endl;

	if(DefaultItemSetList.fail())
	{
		cout << "ERROR: DEFAULT_ITEMSET_LIST.txt IS MISSING - Re-Download this program's files" << endl;
		return 0;
	}
	if(ROMOffsets.fail())
	{
		cout << "ERROR: offsets.txt IS MISSING - Re-Download this program's files" << endl;
		return 0;
	}
	if(!SetOffsets(&ROMOffsets))
	{
		cout << "ERROR: offsets could not be loaded from offsets.txt - Re-Download this program's files" << endl;
		return 0;
	}

	for(int seeker = 1; seeker < argc; ++seeker)
	{
		arguments[seeker-1].assign(argv[seeker]);
	}

	if(arguments[0] == "rawdata")
	{
		if(arguments[1] == "create")
		{
			if(arguments[2].size() != 0)
			{
				ConvertListToBin(arguments[2]); 												//.exe rawdata create "itemsetlist.txt"
			}
			else
			{
				cout << "ERROR: item set text file was not specified" << endl;
				return 0;
			}
		}
		else if(arguments[1] == "extract")
		{
			if(arguments[2].size() != 0)
			{
				ConvertBinToList(arguments[2], &DefaultItemSetList); 							//.exe rawdata extract "extract.bin"
			}
			else
			{
				cout << "ERROR: item set binary file was not specified" << endl;
				return 0;
			}
		}
		else
		{
			cout << "ERROR: command after rawdata is invalid" << endl;
			return 0;
		}
	}
	else if(arguments[0].size() != 0)
	{
		RRTROM.open(arguments[0], ios::in | ios::out | ios::binary);
		if(RRTROM.fail())
		{
			cout << "ERROR: ROM could not be opened" << endl;
			return 0;
		}

		if(arguments[1] == "getdungeonsets")
		{
			if(arguments[2].size() == 0)
			{
				cout << "ERROR: dungeon not specified" << endl;
				return 0;
			}

			unsigned char dungID = GetDungeonID(&RRTROM,arguments[2]);
			if(dungID == 0xFF)
			{
				cout << "ERROR: invalid dungeon name/ID" << endl;
				return 0;
			}

			int result = GetDungeonItemSets(&RRTROM,dungID,arguments[2]);						//.exe "romname.gba" getdungeonsets 00
			switch(result)
			{
			case -1:
				cout << "ERROR: invalid ROM" << endl;
				return 0;
			case -2:
				cout << "ERROR: dungeon ID invalid (floor byte is 0)" << endl;
				return 0;
			case -3:
				cout << "ERROR: dungeon ID invalid (pointer to floor data does not exist)" << endl;
				return 0;
				break;
			case -4:
				cout << "ERROR: data could not be read (either ROM or dungeon ID is invalid)" << endl;
				return 0;
				break;
			default:
				break;
			}
		}
		else if(arguments[1] == "extract")
		{
			if(arguments[2].size() == 0)
			{
				cout << "ERROR: item set ID not specified" << endl;
				return 0;
			}
			unsigned char itemsetID = stoi(arguments[2],nullptr,16);
			ConvertBinFromROMToList(&RRTROM, &DefaultItemSetList, itemsetID); 					//.exe "romname.gba" extract 00
		}
		else if(arguments[1] == "insert")
		{
			if(arguments[2].size() == 0)
			{
				cout << "ERROR: item set list not specified" << endl;
				return 0;
			}

			if(arguments[3].size() != 0)
			{
				unsigned char itemsetID = stoi(arguments[3],nullptr,16);
				ConvertListToInsertInROM(&RRTROM, arguments[2], itemsetID);//.exe "romname.gba" insert "itemsetlist.txt" 00
			}
			else
			{
				cout << "ERROR: must specify set to replace" << endl;
				return 0;
			}
		}
		else
		{
			cout << "ERROR: command after ROM filename is invalid" << endl;
			return 0;
		}
	}
	else
	{
		cout << "ERROR: No valid commands! Please read instructions.txt" << endl;
		return 0;
	}

	return 0;
}


bool SetOffsets(ifstream* ROMOffsets)
{
	string currentline;
	getline(*ROMOffsets,currentline); if(ROMOffsets->fail()) return false;
	getline(*ROMOffsets,currentline,' '); if(ROMOffsets->fail()) return false;
	_ItemSetPointerTable = stoi(currentline,nullptr,16);
	getline(*ROMOffsets,currentline); if(ROMOffsets->fail()) return false;
	getline(*ROMOffsets,currentline,' '); if(ROMOffsets->fail()) return false;
	_DungeonFloorDataPointerTable = stoi(currentline,nullptr,16);
	getline(*ROMOffsets,currentline); if(ROMOffsets->fail()) return false;
	getline(*ROMOffsets,currentline,' '); if(ROMOffsets->fail()) return false;
	_DungeonSizes = stoi(currentline,nullptr,16);
	getline(*ROMOffsets,currentline); if(ROMOffsets->fail()) return false;
	getline(*ROMOffsets,currentline,' '); if(ROMOffsets->fail()) return false;
	_DungeonNamesPointerTable = stoi(currentline,nullptr,16);
	getline(*ROMOffsets,currentline); if(ROMOffsets->fail()) return false;
	getline(*ROMOffsets,currentline,' '); if(ROMOffsets->fail()) return false;
	_HighestDungeonID = stoi(currentline,nullptr,16);

	return true;
}





int ConvertListToBin(string listfilename)
{
	ifstream ItemSetList(listfilename);
	ofstream output("Output.bin", ios::binary);//SUPER IMPORTANT LESSON: ALWAYS HAVE ios::binary WHEN YOU'RE DEALING WITH BINARY FILES. I spent a half an hour trying to figure out why output.bin's contents were different than the ItemSetBin vector's (after spending a couple hours trying to figure out what was wrong with the code in CreateItemSetBinaryData), and finally discovered that a 0D byte was being put before every single 0A byte. This is because 0A is a newline byte, and windows newlines in plain text need 0D 0A, so c++ automatically puts the 0D before every incoming 0A byte when the fstream isn't in binary mode.
	vector<unsigned char> ItemSetBin;

	if(output.fail())
	{
		cout << "ERROR: output file could not be created" << endl;
		return -1;
	}
	if(ItemSetList.fail())
	{
		cout << "ERROR: item set text file could not be opened" << endl;
		return -1;
	}

	string ErrorMassage;
	int result = CreateItemSetBinaryData(&ItemSetList, &ItemSetBin, ErrorMassage);

	if(result == -1)
	{
		cout << "ERROR: item set text file is invalid. Please submit an edited copy of DEFAULT_ITEMSET_LIST.txt" << endl;
		return -1;
	}
	if(result == -2)
	{
		cout << "ERROR: category percentages do not equal 100%" << endl;
		return -1;
	}
	if(result == -3)
	{
		cout << ErrorMassage;
		return -1;
	}

	for(vector<unsigned char>::iterator current = ItemSetBin.begin(); current != ItemSetBin.end(); ++current)
	{
		output << *current;
	}
	cout << "Successfully created item set data - see Output.bin" << endl;
	return 0;
}

int ConvertListToInsertInROM(fstream* RRTROM, string listfilename, unsigned char itemsetID)
{
	ifstream ItemSetList(listfilename);
	vector<unsigned char> ItemSetBin;

	if(ItemSetList.fail())
	{
		cout << "ERROR: item set list could not be opened" << endl;
		return -1;
	}

	string ErrorMassage;
	int result = CreateItemSetBinaryData(&ItemSetList, &ItemSetBin, ErrorMassage);

	if(result == -1)
	{
		cout << "ERROR: item set text file is invalid. Please submit an edited copy of DEFAULT_ITEMSET_LIST.txt" << endl;
		return -1;
	}
	if(result == -2)
	{
		cout << "ERROR: category percentages do not equal 100%" << endl;
		return -1;
	}
	if(result == -3)
	{
		cout << ErrorMassage;
		return -1;
	}


	unsigned int itemsetadres = seekROMToItemSetAddress(RRTROM, itemsetID);
	if(itemsetadres == 0) {cout << "ERROR: invalid ROM" << endl; return -1;}
	if(itemsetadres == 0xFFFFFFFF) {cout << "ERROR: item set ID invalid (pointer to item set does not exist)" << endl; return -1;}

	result = getBinItemSetSize(RRTROM);
	if(result == -1) {cout << "ERROR: invalid item ID (item set to replace has too many entries, implying non-itemset data was being read during its size test)" << endl; return -1;}
	if(result == -2) {cout << "ERROR: invalid ROM" << endl; return -1;}

	if((ItemSetBin.size() % 2 == 1) || (result % 2 == 1))
	{
		cout << "ERROR: somehow, an item set's size was odd?????? Please report this bug" << endl;
		cout << "Size of your item set: " << ItemSetBin.size() << endl;
		cout << "Size of item set " << (int)itemsetID << ": " << result << endl;
		return -8413615;
	}

	if((int)ItemSetBin.size() > result)
	{
		cout << "ERROR: Your item set's size is too large to be put in place of item set " << (int)itemsetID << endl;
		cout << "Size of your item set: " << ItemSetBin.size() << endl;
		cout << "Size of item set " << (int)itemsetID << ": " << result << endl;
		cout << "Read the instructions for tips on how to lower the size of your item set." << endl;
		cout << "Alternatively, you can use [rawdata create] to create the binary of your list, use a hex editor to put that binary in empty space in the ROM (0x280000-0x300000 area US), and then repoint an item set's pointer to the location of your new item set." << endl;
		cout << "Location of pointer to item set " << hex << (int)itemsetID << ": " << (_ItemSetPointerTable +(itemsetID*4)) << endl;
		return -1;
	}

	while((int)ItemSetBin.size() < result)
	{
		ItemSetBin.push_back(0);
		ItemSetBin.push_back(0);
	}


	RRTROM->seekp(itemsetadres);
	for(vector<unsigned char>::iterator current = ItemSetBin.begin(); current != ItemSetBin.end(); ++current)
	{
		RRTROM->put(*current);
	}
	if(RRTROM->eof()) cout << "WARNING: End of file reached during data insertion. ROM size may be different." << endl;
	cout << "Successfully inserted your item set in the ROM!" << endl;

	return 0;
}

int CreateItemSetBinaryData(ifstream* ItemSetList, vector<unsigned char>* ItemSetBin, string& ErrorMessages)
{
	bool runescape = false;
	int currententry = 0;
	int currentcategory = 0;
	int skippedentries = 0;
	int totalcategorypercents = 0;
	bool nonzerocategories[_MaxCategories] = {0};
	int itemperecnts[_MaxCategories] = {0};

	while((currententry < _MaxItemSetEntries) && !runescape)
	{
		string aline;
		string numbstr;
		int numb = 0;
		unsigned char byte1 = 0;
		unsigned char byte2 = 0;

		getline(*ItemSetList,aline);
		if(ItemSetList->fail()) return -1;
		numbstr.assign(aline,0,3);
		numbstr.append(string(aline,4,2));
		if(numbstr.size() != 5) runescape = true;
		else numb = stoi(numbstr,nullptr); //numb = percentage * 100

		if(numb == 0) ++skippedentries;
		else
		{
			if(skippedentries > 0)
			{
				int halfword = 0x7530;
				halfword += skippedentries;
				if(halfword >= 0x10000) return -1;
				byte1 = halfword & 0xFF;
				byte2 = (halfword & 0xFF00) >> 8;
				ItemSetBin->push_back(byte1);
				ItemSetBin->push_back(byte2);
				skippedentries = 0;
			}

			if(currententry < _MaxCategories)
			{
				totalcategorypercents += numb;
				nonzerocategories[currententry] = true;
				byte1 = totalcategorypercents & 0xFF;
				byte2 = (totalcategorypercents & 0xFF00) >> 8;
			}
			else
			{
				int actualcurrentcategory = currentcategory;
				if(string().assign(aline,aline.size()-3,3) == "Orb") actualcurrentcategory = 9;//Orbs
				if(currententry == 12) actualcurrentcategory = 2;//Nothing exception
				if(currententry == 66) actualcurrentcategory = 8;//Wish Stone exception
				itemperecnts[actualcurrentcategory] += numb;
				byte1 = itemperecnts[actualcurrentcategory] & 0xFF;
				byte2 = (itemperecnts[actualcurrentcategory] & 0xFF00) >> 8;
			}
			ItemSetBin->push_back(byte1);
			ItemSetBin->push_back(byte2);
		}

		++currententry;
		currentcategory = updateCurrentCategory(currententry);
	}

	if(skippedentries > 0)
	{
		unsigned char byte1 = 0;
		unsigned char byte2 = 0;
		int halfword = 0x7530;
		halfword += skippedentries;
		if(halfword >= 0x10000) return -1;
		byte1 = halfword & 0xFF;
		byte2 = (halfword & 0xFF00) >> 8;
		ItemSetBin->push_back(byte1);
		ItemSetBin->push_back(byte2);
		skippedentries = 0;
	}


	if(totalcategorypercents != 10000) return -2;

	for(int checkerrors = 0; checkerrors < _MaxCategories; ++checkerrors)
	{
		string catname;
		switch(checkerrors)
		{
		case  0: catname =  "Throwables"; break;
		case  1: catname =  "Rocks"; break;
		case  2: catname =  "Berries + Seeds"; break;
		case  3: catname =  "Food + Gummis"; break;
		case  4: catname =  "Hold Items"; break;
		case  5: catname =  "TMs"; break;
		case  6: catname =  "Money"; break;
		case  7: catname =  "Unused"; break;
		case  8: catname =  "Misc"; break;
		case  9: catname =  "Orbs"; break;
		case 10: catname =  "Machines"; break;
		case 11: catname =  "Used TM"; break;
		default: break;
		}

		if(nonzerocategories[checkerrors] == true)
		{
			if(itemperecnts[checkerrors] != 10000)
			{
				int hundrids = itemperecnts[checkerrors]/100;
				string overone = to_string(hundrids);
				string underone = to_string(itemperecnts[checkerrors] - (hundrids*100));
				ErrorMessages.append("ERROR: Items in " + catname + " category do not add up to 100% - they add up to " + overone + "." + underone + '\n');
			}
		}
		else
		{
			if(itemperecnts[checkerrors] != 0)
			{
				ErrorMessages.append("ERROR: Items in the " + catname + " category exist, but the " + catname + " category is 0%." + '\n');
			}
		}
	}
	if(!ErrorMessages.empty()) return -3;

	return 0;
}





int ConvertBinToList(string binfilename, ifstream* DefaultItemSetList)
{
	fstream ItemSetBin(binfilename, ios::in | ios::binary);
	ofstream output("Output.txt");
	if(output.fail())
	{
		cout << "ERROR: output file could not be created" << endl;
		return -1;
	}
	if(ItemSetBin.fail())
	{
		cout << "ERROR: item set binary file could not be opened" << endl;
		return -1;
	}

	int result = ExtractItemSetBinaryData(&ItemSetBin, &output, DefaultItemSetList);

	switch(result)
	{
	case -1:
		cout << "ERROR: item set is invalid, Output.txt may be incomplete" << endl;
		return -1;
		break;
	case -2:
		cout << "ERROR: either the item set is invalid or DEFAULT_ITEMSET_LIST was edited, Output.txt may be incomplete" << endl;
		return -1;
		break;
	default:
		cout << "Successfully created item set list - see Output.txt" << endl;
		break;
	}
	return 0;
}

int ConvertBinFromROMToList(fstream* RRTROM, ifstream* DefaultItemSetList, unsigned char itemsetID)
{
	ofstream output("Output.txt");
	if(output.fail())
	{
		cout << "ERROR: output file could not be created" << endl;
		return -1;
	}

	unsigned int itemsetadres = seekROMToItemSetAddress(RRTROM, itemsetID);
	if(itemsetadres == 0) {cout << "ERROR: invalid ROM" << endl; return -3;}
	if(itemsetadres == 0xFFFFFFFF) {cout << "ERROR: item set ID invalid (pointer to item set does not exist)" << endl; return -4;}

	int result = ExtractItemSetBinaryData(RRTROM, &output, DefaultItemSetList);

	switch(result)
	{
	case -1:
		cout << "ERROR: item set is invalid, Output.txt may be incomplete" << endl;
		return -1;
		break;
	case -2:
		cout << "ERROR: either the item set is invalid or DEFAULT_ITEMSET_LIST was edited, Output.txt may be incomplete" << endl;
		return -1;
		break;
	default:
		cout << "Successfully created item set list - see Output.txt" << endl;
		cout << "Item set position in ROM is " << hex << itemsetadres << ", and its pointer is at " << (_ItemSetPointerTable +(itemsetID*4)) << endl;
		break;
	}
	return 0;
}

int ExtractItemSetBinaryData(fstream* ItemSetBin, ofstream* OutputItemSetList, ifstream* DefaultItemSetList)
{
	int currententry = 0;
	int currentcategory = 0;
	int totalcategorypercents = 0;
	int itemperecnts[_MaxCategories] = {0};

	while(currententry < _MaxItemSetEntries)
	{
		unsigned char byte1 = ItemSetBin->get();
		unsigned char byte2 = ItemSetBin->get();
		if(ItemSetBin->fail()) return -1;
		int numb = byte1 + (byte2 * 0x100);

		if(numb >= 0x7530)
		{
			numb -= 0x7530;
			while(numb > 0)
			{
				string theline;
				getline(*DefaultItemSetList,theline);
				if(DefaultItemSetList->fail()) return -2;
				if(theline.size() != 0) *OutputItemSetList << theline << endl;
				--numb;
				++currententry;
				currentcategory = updateCurrentCategory(currententry);
			}
		}
		else
		{
			string theline;
			string therestoftheline;
			int actualpercent = 0;
			int actualcategory = currentcategory;
			getline(*DefaultItemSetList,theline);
			if(DefaultItemSetList->eof() || theline.empty()) return -2;
			if(currententry < _MaxCategories)
			{
				actualpercent = numb - totalcategorypercents;
				totalcategorypercents = numb;
			}
			else
			{
				if(string().assign(theline,theline.size()-3,3) == "Orb") actualcategory = 9;
				if(currententry == 12) actualcategory = 2;//Nothing exception
				if(currententry == 66) actualcategory = 8;//Wish Stone exception
				actualpercent = numb - itemperecnts[actualcategory];
				itemperecnts[actualcategory] = numb;
			}
			if(actualpercent == 0)
			{
				cout << "WARNING: Item set entry " << dec << currententry << " is included in the set, but has a 0% chance to be picked???" << endl;
			}

			int percentwithoutdecimals = actualpercent/100;
			int percentsdecimals = actualpercent -(percentwithoutdecimals*100);
			therestoftheline.assign(theline,6,theline.size()-6);
			theline.clear();
			if(percentwithoutdecimals < 100) theline.append("0");
			if(percentwithoutdecimals < 10) theline.append("0");
			if(percentwithoutdecimals < 1) theline.append("0");
			else theline.append(to_string(percentwithoutdecimals));
			theline.append(".");
			if(percentsdecimals < 10) theline.append("0");
			theline.append( to_string(percentsdecimals) );
			theline.append(therestoftheline);
			*OutputItemSetList << theline << endl;

			++currententry;
			currentcategory = updateCurrentCategory(currententry);
		}
	}

	while(!DefaultItemSetList->eof())
	{
		string theline;
		getline(*DefaultItemSetList,theline);
		if(theline.size() != 0) *OutputItemSetList << theline << endl;
	}

	if(currententry > _MaxItemSetEntries)
	{
		cout << "WARNING: Item set skips past entry " << dec << _MaxItemSetEntries << ", to " << currententry << ": item set is invalid, and an adjacent item set's data may be included in output." << endl;
	}
	if((totalcategorypercents != 0) && (totalcategorypercents != 10000)) cout << "WARNING: Categories do not add up to 0/100%, item set is invalid." << endl;
	for(int checkerrors = 0; checkerrors < _MaxCategories; ++checkerrors)
	{
		string catname;
		switch(checkerrors)
		{
		case  0: catname =  "Throwables"; break;
		case  1: catname =  "Rocks"; break;
		case  2: catname =  "Berries + Seeds"; break;
		case  3: catname =  "Food + Gummis"; break;
		case  4: catname =  "Hold Items"; break;
		case  5: catname =  "TMs"; break;
		case  6: catname =  "Money"; break;
		case  7: catname =  "Unused"; break;
		case  8: catname =  "Misc"; break;
		case  9: catname =  "Orbs"; break;
		case 10: catname =  "Machines"; break;
		case 11: catname =  "Used TM"; break;
		default: break;
		}

		if((itemperecnts[checkerrors] != 0) && (itemperecnts[checkerrors] != 10000))
				cout << "WARNING: Items in category " << catname << "do not add up to 0/100%, item set is invalid." << endl;
	}

	return 0;
}




unsigned char GetDungeonID(fstream* RRTROM, string& arguing)
{
	unsigned int dungID = 0xFF;
	if(string().assign(arguing,0,2) == "ID") dungID = stoi(string().assign(arguing,2,arguing.size()-2),nullptr,16); //If the argument was an ID, then the function continues to find the dungeon's name

	for(unsigned char currentID = 0; currentID <= _HighestDungeonID; ++currentID)
	{
		string currentDungName;
		RRTROM->seekg(_DungeonNamesPointerTable +(currentID*8));
		if(RRTROM->fail()) return 0xFF;
		unsigned char UnprocessedAddress[4] = {0,0,0,0};
		RRTROM->read((char*)&UnprocessedAddress, 4);
		if(RRTROM->fail()) return 0xFF;
		unsigned int dungnameadres = (UnprocessedAddress[3]*0x1000000) +(UnprocessedAddress[2]*0x10000) +(UnprocessedAddress[1]*0x100) +UnprocessedAddress[0];
		if(dungnameadres < 0x08000000) return 0xFF;
		dungnameadres -= 0x08000000;
		RRTROM->seekg(dungnameadres);

		getline(*RRTROM,currentDungName,(char)0);

		if(dungID != 0xFF)
		{
			if(dungID == currentID)
			{
				arguing = currentDungName;
				return currentID;
			}
		}
		else
		{
			if(arguing == currentDungName) return currentID;
		}
	}

	return 0xFF;
}

int GetDungeonItemSets(fstream* RRTROM, unsigned char dungID, string dungName)
{
	RRTROM->seekg(_DungeonSizes + dungID);
	if(RRTROM->fail()) return -1;
	unsigned char dungeonfloors = RRTROM->peek();
	if(dungeonfloors == 0) return -2;
	RRTROM->seekg(_DungeonFloorDataPointerTable + (dungID*4));
	if(RRTROM->fail()) return -1;
	unsigned char UnprocessedAddress[4] = {0,0,0,0};
	RRTROM->read((char*)&UnprocessedAddress, 4);
	if(RRTROM->fail()) return -1;
	unsigned int dungflordatadres = (UnprocessedAddress[3]*0x1000000) +(UnprocessedAddress[2]*0x10000) +(UnprocessedAddress[1]*0x100) +UnprocessedAddress[0];
	if(dungflordatadres < 0x08000000) return -3;
	dungflordatadres -= 0x08000000;
	cout << "Retrieving the item set IDs used on each floor in a dungeon with ID " << hex << (int)dungID << endl;
	cout << "Dungeon name: " << dungName << endl;
	cout << "Dungeon floors: " << dec << (dungeonfloors - 1) << endl;
	cout << "Dungeon Floor Data Offset: " << hex << dungflordatadres << endl;
	cout << "(To manually change the item sets the floors use, go to the above offset in the ROM - see instructions for more details)" << endl;
	cout << "(All item set IDs are in hexadecimal, and in order of: main items - kecleon items - monster house items - hidden wall items)" << endl;

	for(unsigned char curdungflor = 1; curdungflor < dungeonfloors; ++curdungflor)
	{
		unsigned char readbyte1 = 0;
		unsigned char readbyte2 = 0;
		unsigned char readbyte3 = 0;
		unsigned char readbyte4 = 0;
		RRTROM->seekg( dungflordatadres +(0x10*curdungflor) +6);
		if(RRTROM->fail()) return -4;
		readbyte1 = RRTROM->get();
		RRTROM->seekg( dungflordatadres +(0x10*curdungflor) +8);
		if(RRTROM->fail()) return -4;
		readbyte2 = RRTROM->get();
		RRTROM->seekg( dungflordatadres +(0x10*curdungflor) +0xA);
		if(RRTROM->fail()) return -4;
		readbyte3 = RRTROM->get();
		RRTROM->seekg( dungflordatadres +(0x10*curdungflor) +0xC);
		if(RRTROM->fail()) return -4;
		readbyte4 = RRTROM->get();
		cout << "F" << dec << (int)curdungflor << "(" << hex << (int)curdungflor << ")" << ": " << (int)readbyte1 << " " << (int)readbyte2 << " " << (int)readbyte3 << " " << (int)readbyte4 << endl;
	}

	return 0;
}


unsigned int seekROMToItemSetAddress(fstream* RRTROM, unsigned char itemsetID)
{
	RRTROM->seekg(_ItemSetPointerTable +(itemsetID*4));
	if(RRTROM->fail()) return 0;
	unsigned char UnprocessedAddress[4] = {0,0,0,0};
	RRTROM->read((char*)&UnprocessedAddress, 4);
	if(RRTROM->fail()) return 0;
	unsigned int itemsetadres = (UnprocessedAddress[3]*0x1000000) +(UnprocessedAddress[2]*0x10000) +(UnprocessedAddress[1]*0x100) +UnprocessedAddress[0];
	if(itemsetadres < 0x08000000) return 0xFFFFFFFF;
	itemsetadres -= 0x08000000;
	RRTROM->seekg(itemsetadres);
	if(RRTROM->fail()) return 0;

	return itemsetadres;
}


int getBinItemSetSize(fstream* ItemSetBin)
{
	bool donereading = false;
	int isetsize = 0;
	int currententry = 0;

	while(!donereading)
	{
		unsigned char byte1 = ItemSetBin->get();
		unsigned char byte2 = ItemSetBin->get();
		if(ItemSetBin->fail()) return -2;
		int numb = byte1 + (byte2 * 0x100);

		if(numb >= 0x7530)
		{
			numb -= 0x7530;
			currententry += numb;
		}
		else
		{
			++currententry;
		}
		isetsize += 2;

		if(currententry == _MaxItemSetEntries) donereading = true;
		if(currententry > _MaxItemSetEntries) return -1;
	}

	donereading = false;
	while(!donereading)
	{
		unsigned char byte1 = ItemSetBin->get();
		unsigned char byte2 = ItemSetBin->get();
		if(ItemSetBin->fail()) donereading = true;
		int numb = byte1 + (byte2 * 0x100);

		if(numb != 0) donereading = true;
		else
		{
			isetsize += 2;
			if(isetsize >= _MaxItemSetEntries*2) donereading = true;
		}
	}

	return isetsize;
}

int updateCurrentCategory(int currententry)
{
	if((currententry >=  13) && (currententry <   19)) return  0;//Throwables
	if((currententry >=  19) && (currententry <   21)) return  1;//Rocks
	if((currententry >=  21) && (currententry <   65)) return  4;//Hold Items
	if((currententry >=  65) && (currententry <   94)) return  2;//Berries + Seeds
	if((currententry >=  94) && (currententry <  117)) return  3;//Food + Gummis
	if((currententry >= 117) && (currententry <  118)) return  6;//Money
	if((currententry >= 118) && (currententry <  136)) return  8;//Misc
	if((currententry >= 136) && (currententry <  137)) return 11;//Used TM
	if((currententry >= 137) && (currententry <  244)) return  5;//TMs and Orbs
	if((currententry >= 244)) 						   return 10;//Machines
	return 0;
}
















