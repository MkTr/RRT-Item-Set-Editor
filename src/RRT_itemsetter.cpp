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
int ConvertListToInsertInROM(fstream* RRTROM, ifstream* DefaultItemSetList, string listfilename, unsigned char itemsetID);
int CreateItemSetBinaryData(ifstream* ItemSetList, vector<unsigned char>* ItemSetBin);
int CheckForAdditionalListWarnings(ifstream* ItemSetList);

int ConvertBinToList(string binfilename, ifstream* DefaultItemSetList);
int ConvertBinFromROMToList(fstream* RRTROM, ifstream* DefaultItemSetList, unsigned char itemsetID);
int ExtractItemSetBinaryData(fstream* ItemSetBin, ofstream* OutputItemSetList, ifstream* DefaultItemSetList);

int GetDungeonItemSets(fstream* RRTROM, unsigned char dungID);
unsigned int seekROMToItemSetAddress(fstream* RRTROM, unsigned char itemsetID);
int getBinItemSetSize(fstream* ItemSetBin, ifstream* DefaultItemSetList, int& testeroni);


int _ItemSetPointerTable;			//US: 4CB56C
int _DungeonFloorDataPointerTable;	//US: 4A9E74
int _DungeonSizes;					//US: 1077A8



int main(int argc, char *argv[])
{
	string arguments[4];
	fstream RRTROM;
	ifstream DefaultItemSetList("DEFAULT_ITEMSET_LIST.txt");
	ifstream ROMOffsets("offsets.txt");

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
				cout << "ERROR: dungeon ID not specified" << endl;
				return 0;
			}
			unsigned char dungID = stoi(arguments[2],nullptr,16);
			int result = GetDungeonItemSets(&RRTROM,dungID); 									//.exe "romname.gba" getdungeonsets 00
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
			if(arguments[0] == arguments[2])
			{
				cout << "ERROR: please name your ROM something other than Output.gba" << endl;
				return 0;
			}

			if(arguments[3].size() != 0)
			{
				unsigned char itemsetID = stoi(arguments[3],nullptr,16);
				ConvertListToInsertInROM(&RRTROM, &DefaultItemSetList, arguments[2], itemsetID);						//.exe "romname.gba" insert "itemsetlist.txt" 00
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

	return true;
}





int ConvertListToBin(string listfilename)
{
	ifstream ItemSetList(listfilename);
	ofstream output("Output.bin");
	vector<unsigned char> ItemSetBin;

	if(output.fail())
	{
		cout << "ERROR: output file could not be created" << endl;
		return -4;
	}
	if(ItemSetList.fail())
	{
		cout << "ERROR: item set text file could not be opened" << endl;
		return -1;
	}

	int result = CreateItemSetBinaryData(&ItemSetList, &ItemSetBin);
	if(result >= 0) CheckForAdditionalListWarnings(&ItemSetList);

	if(result == -1)
	{
		cout << "ERROR: item set text file is invalid. Please submit an edited copy of DEFAULT_ITEMSET_LIST.txt" << endl;
		return -1;
	}
	if(result == -2)
	{
		cout << "ERROR: category percentages do not equal 100.00" << endl;
		return -2;
	}
	if(result == -3)
	{
		cout << "ERROR: item percentages in a category do not add up to 100.00" << endl;
		return -3;
	}

	for(vector<unsigned char>::iterator current = ItemSetBin.begin(); current != ItemSetBin.end(); ++current)
	{
		output << *current;
	}
	cout << "Successfully created item set data - see Output.bin" << endl;
	return 0;
}

int ConvertListToInsertInROM(fstream* RRTROM, ifstream* DefaultItemSetList, string listfilename, unsigned char itemsetID)
{
	ifstream ItemSetList(listfilename);
	vector<unsigned char> ItemSetBin;

	if(ItemSetList.fail())
	{
		cout << "ERROR: item set list could not be opened" << endl;
		return -5;
	}

	int result = CreateItemSetBinaryData(&ItemSetList, &ItemSetBin);
	if(result >= 0) CheckForAdditionalListWarnings(&ItemSetList);

	if(result == -1)
	{
		cout << "ERROR: item set text file is invalid. Please submit an edited copy of DEFAULT_ITEMSET_LIST.txt" << endl;
		return -1;
	}
	if(result == -2)
	{
		cout << "ERROR: category percentages do not equal 100.00" << endl;
		return -2;
	}
	if(result == -3)
	{
		cout << "ERROR: item percentages in a category do not add up to 100.00" << endl;
		return -3;
	}


	unsigned int itemsetadres = seekROMToItemSetAddress(RRTROM, itemsetID);
	if(itemsetadres == 0) {cout << "ERROR: invalid ROM" << endl; return -6;}
	if(itemsetadres == 0xFFFFFFFF) {cout << "ERROR: item set ID invalid (pointer to item set does not exist)" << endl; return -7;}

	int testeroni = 0;
	result = getBinItemSetSize(RRTROM, DefaultItemSetList, testeroni);
	if(result == -1) {cout << "ERROR: invalid item ID (item set to replace is too large, implying non-itemset data was being read during its size test)" << endl; return -7;}
	if(result == -2) {cout << "ERROR: invalid ROM" << endl; return -6;}

	if((ItemSetBin.size() % 2 == 1) || (result % 2 == 1))
	{
		cout << "ERROR: somehow, an item set's size was odd?????? Please report this bug" << endl;
		cout << "Size of your item set: " << ItemSetBin.size() << endl;
		cout << "Size of item set " << (int)itemsetID << ": " << result << endl;
		return -8413615;
	}

	if((int)ItemSetBin.size() > result)
	{
		cout << "ERROR: Your item set's size is too large to be put in place of item set " << itemsetID << endl;
		cout << "Size of your item set: " << ItemSetBin.size() << endl;
		cout << "Size of item set " << (int)itemsetID << ": " << result << endl;
		cout << "Read the instructions for tips on how to lower the size of your item set." << endl;
		cout << "Alternatively, you can use [rawdata create] to create the binary of your list, use a hex editor to put that binary in empty space in the ROM (0x280000-0x300000 area US), and then repoint an item set's pointer to the location of your new item set." << endl;
		cout << "Location of pointer to item set " << (int)itemsetID << ": " << hex << (_ItemSetPointerTable +(itemsetID*4)) << endl;
		return -8;
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

int CreateItemSetBinaryData(ifstream* ItemSetList, vector<unsigned char>* ItemSetBin)
{
	int totalpercentage = 0;
	int totalORBpercentage = 0;
	int skippedlines = 0;
	bool categoriesdone = false;
	bool orbsdone = false;
	int currentcategory = 0;
	int totalcategories = 0;

	while(1)
	{
		string aline;
		string numbstr;
		int numb;
		unsigned char byte1;
		unsigned char byte2;
		bool thisanorb = false;

		if((totalORBpercentage == 10000) && (!orbsdone)) //I had to separate orbs from the other categories because they're mixed in with TMs. The correct thing to do is have a separate variable to keep track of every single category's percentage, but I didn't want to rewrite all my code - though I'll need to if it turns out other item categories are mixed with each other. (The only way to notice this is if you extract an item set from the game and notice items in a category don't add up to 100% or have negative values).
		{
			orbsdone = true;
			if((currentcategory+orbsdone) == totalcategories) return 0;
		}
		if(totalpercentage == 10000)
		{
			if(!categoriesdone) //The first time the set reaches 100%, that means the category chances are done.
			{
				totalpercentage = 0;
				categoriesdone = true;
			}
			else				//Every time after that means the items in a category have been completed (assuming items are grouped based on their category, except for orbs).
			{
				totalpercentage = 0;
				++currentcategory;
			}

			if( (currentcategory+orbsdone) == totalcategories ) return 0;
		}

		getline(*ItemSetList,aline);

		if((totalpercentage > 10000) || ItemSetList->eof())
		{
			if(!categoriesdone) return -2;
			else return -3;
		}
		if(totalORBpercentage > 10000) return -3;
		if(aline.empty()) return -1;

		if(string().assign(aline,7,7) == "Nothing" && !categoriesdone)	//I use the Nothing item to check for errors in the user's list: by now, if categories haven't been completed, or if they have and they're over 100%, there's a problem.
		{
			if(!categoriesdone) return -2;
			if(totalpercentage > 0) return -2;
			if(currentcategory > 0) return -2;
		}

		if(string().assign(aline,aline.size()-3,3) == "Orb") thisanorb = true; //This is why items in the orbs category NEED to end with "orb" in default_itemset_list. I actually can't get over the fact that orbs and TMs are mixed together like how does that even work in the game's code why does it exist just to spite me

		numbstr.assign(aline,0,3);
		numbstr.append(string(aline,4,2));
		if(numbstr.size() != 5) return -1;
		numb = stoi(numbstr,nullptr); //numb = percentage * 100

		if(numb == 0) ++skippedlines;
		else
		{
			if(!categoriesdone) ++totalcategories;
			if(skippedlines > 0)
			{	//Inserting the "skip to line" command in the item set before continuing
				int halfword = 0x7530;
				halfword += skippedlines;
				if(halfword >= 0x10000) return -1;
				byte1 = halfword & 0xFF;
				byte2 = (halfword & 0xFF00) >> 8;
				ItemSetBin->push_back(byte1);
				ItemSetBin->push_back(byte2);
				skippedlines = 0;
			}
			if(thisanorb)
			{
				totalORBpercentage += numb;
				byte1 = totalORBpercentage & 0xFF;
				byte2 = (totalORBpercentage & 0xFF00) >> 8;
			}
			else
			{
				totalpercentage += numb;
				byte1 = totalpercentage & 0xFF;
				byte2 = (totalpercentage & 0xFF00) >> 8;
			}
			ItemSetBin->push_back(byte1);
			ItemSetBin->push_back(byte2);
		}
	}

	return -1;
}

int CheckForAdditionalListWarnings(ifstream* ItemSetList)
{
	int itemsmissed = 0;
	string misseditem;

	while(!ItemSetList->eof())
	{
		string aline;
		getline(*ItemSetList,aline);
		if(!aline.empty())
		{
			if( (string().assign(aline,0,3) != "000") || (string(aline,4,2) != "00") )
			{
				++itemsmissed;
				if(itemsmissed == 1) misseditem = string().assign(aline,7,aline.size()-7);
			}
		}
	}

	if(itemsmissed > 0)
	{
		int tabremover = 0;
		tabremover = misseditem.find('\t');
		if(tabremover > 0) misseditem.resize(tabremover);
		cout << "WARNING: Item set was created, but ";
		cout << misseditem;
		if(itemsmissed > 1) cout << " and items beyond it were ";
		else cout << " was ";
		cout << "ignored, as 100% probability was already reached in all categories before " << misseditem << " was read." << endl;
	}

	return 0;
}




int ConvertBinToList(string binfilename, ifstream* DefaultItemSetList)
{
	fstream ItemSetBin(binfilename, ios::in | ios::binary);
	ofstream output("Output.txt");
	if(output.fail())
	{
		cout << "ERROR: output file could not be created" << endl;
		return -3;
	}
	if(ItemSetBin.fail())
	{
		cout << "ERROR: item set binary file could not be opened" << endl;
		return -4;
	}

	int result = ExtractItemSetBinaryData(&ItemSetBin, &output, DefaultItemSetList);

	switch(result)
	{
	case -1:
		cout << "ERROR: category percentages do not equal 100.00, item set is invalid and Output.txt may be incomplete" << endl;
		return -1;
		break;
	case -2:
		cout << "ERROR: item percentages in a category do not equal 100.00, item set is invalid and Output.txt may be incomplete" << endl;
		return -2;
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
		return -3;
	}

	unsigned int itemsetadres = seekROMToItemSetAddress(RRTROM, itemsetID);
	if(itemsetadres == 0) {cout << "ERROR: invalid ROM" << endl; return -3;}
	if(itemsetadres == 0xFFFFFFFF) {cout << "ERROR: item set ID invalid (pointer to item set does not exist)" << endl; return -4;}

	int result = ExtractItemSetBinaryData(RRTROM, &output, DefaultItemSetList);

	switch(result)
	{
	case -1:
		cout << "ERROR: category percentages do not equal 100.00, item set is invalid and Output.txt may be incomplete" << endl;
		return -1;
		break;
	case -2:
		cout << "ERROR: item percentages in a category do not equal 100.00, item set is invalid and Output.txt may be incomplete" << endl;
		return -2;
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
	int lastpercentage = 0;
	int lastORBpercentage = 0;
	bool categoriesdone = false;
	int totalcategories = 0;
	int currentcategory = 0;
	bool orbsdone = false;
	bool itemsdone = false;
	bool warningraised = false;

	while(!ItemSetBin->eof() && !itemsdone)
	{
		unsigned char byte1 = ItemSetBin->get();
		unsigned char byte2 = ItemSetBin->get();
		if(ItemSetBin->fail()) return (categoriesdone ? -2 : -1);
		int numb = byte1 + (byte2 * 0x100);

		if(numb >= 0x7530)
		{
			numb -= 0x7530;
			while(numb > 0)
			{
				string theline;
				getline(*DefaultItemSetList,theline);
				if(DefaultItemSetList->eof())
				{
					if(!categoriesdone) return -1;
					return -2;
				}
				if(theline.size() != 0)
				{
					if(string().assign(theline,7,7) == "Nothing")
					{
						if(!categoriesdone) return -1;
						if(lastpercentage > 0) return -1;
						if(currentcategory > 0) return -1;
					}
					*OutputItemSetList << theline << endl;
				}
				--numb;
			}
		}
		else
		{
			if(!categoriesdone) ++totalcategories;
			string theline;
			string therestoftheline;
			getline(*DefaultItemSetList,theline);
			if(DefaultItemSetList->eof() || theline.empty())
			{
				if(!categoriesdone) return -1;
				return -2;
			}
			int actualpercent;
			if(string().assign(theline,theline.size()-3,3) == "Orb")
			{
				actualpercent = numb -lastORBpercentage;
				lastORBpercentage += actualpercent;
			}
			else
			{
				actualpercent = numb -lastpercentage;
				lastpercentage += actualpercent;
			}
			if(actualpercent == 0)
			{
				if(categoriesdone) cout << "WARNING: An item included in the set has a percentage of 0???" << endl;
				else cout << "WARNING: A category included in the set has a percentage of 0???" << endl;
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
			if(string().assign(theline,7,7) == "Nothing")
			{
				if(!categoriesdone) return -1;
				if(lastpercentage > 0) return -1;
				if(currentcategory > 0) return -1;
			}
		}

		if((lastpercentage > 10000) && !warningraised)
		{
			if(!categoriesdone) cout << "WARNING: somehow, category percentages are OVER 100" << endl;
			else cout << "WARNING: somehow, item percentages in a category are OVER 100" << endl;
			warningraised = true;
		}
		if((lastORBpercentage > 10000) && !warningraised)
		{
			cout << "WARNING: somehow, orb percentages are OVER 100" << endl;
			warningraised = true;
		}

		if((lastORBpercentage >= 10000) && (!orbsdone))
		{
			orbsdone = true;
			if((currentcategory+orbsdone) == totalcategories) itemsdone = true;
		}

		if(lastpercentage >= 10000)
		{
			if(!categoriesdone)
			{
				categoriesdone = true;
				warningraised = false;
				lastpercentage = 0;
			}
			else
			{
				lastpercentage = 0;
				++currentcategory;
				if((currentcategory+orbsdone) == totalcategories) itemsdone = true;
			}
		}

		if(itemsdone)
		{
			while(!DefaultItemSetList->eof())
			{
				string theline;
				getline(*DefaultItemSetList,theline);
				if(theline.size() != 0) *OutputItemSetList << theline << endl;
			}
		}

	}

	if(!categoriesdone) return -1;
	if(!itemsdone) return -2;
	return 0;
}




int GetDungeonItemSets(fstream* RRTROM, unsigned char dungID)
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
	cout << "Dungeon floors: " << dec << (dungeonfloors - 1) << endl;
	cout << "Dungeon Floor Data Offset: " << hex << dungflordatadres << endl;
	cout << "(To manually change the item sets the floors use, go to the above offset in the ROM - see instructions for more details)" << endl;
	cout << "(All item set IDs below are in hexadecimal, and in order of: main items - kecleon items - monster house items - hidden wall items)" << endl;

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
		cout << "F" << dec << (int)curdungflor << ": " << hex << (int)readbyte1 << " " << (int)readbyte2 << " " << (int)readbyte3 << " " << (int)readbyte4 << endl;
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


int getBinItemSetSize(fstream* ItemSetBin, ifstream* DefaultItemSetList, int& testeroni)
{
	bool donereading = false;
	int isetsize = 0;
	bool categoriesdone = false;
	int nonzerocategories = 0;
	int completedcategories = 0;
	int MaximumCategories = 0;
	int MaximumPossibleSize = 0;
	int currententry = 0;
	testeroni = 5555;

	bool gettingmaximumcategories = true;
	while(!DefaultItemSetList->eof())
	{
		string aline;
		getline(*DefaultItemSetList,aline);

		if(!aline.empty())
		{
			if(gettingmaximumcategories)
			{
				if(string().assign(aline,7,7) == "Nothing") gettingmaximumcategories = false;
				else ++MaximumCategories;
			}
			MaximumPossibleSize += 2;
		}
	}

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
		else if(numb == 0x2710)
		{
			if(!categoriesdone)
			{
				categoriesdone = true;
				++nonzerocategories;
			}
			else
			{
				++completedcategories;
				if(completedcategories == nonzerocategories) donereading = true;
			}
			++currententry;
		}
		else
		{
			if(!categoriesdone) ++nonzerocategories;
			++currententry;
		}
		isetsize += 2;

		if(currententry >= MaximumCategories) categoriesdone = true;
		if(currententry*2 >= MaximumPossibleSize) return -1;
		if(isetsize > MaximumPossibleSize) return -1;
	}

	donereading = false;
	while(!donereading)
	{
		unsigned char byte1 = ItemSetBin->get();
		unsigned char byte2 = ItemSetBin->get();
		if(ItemSetBin->fail()) return -2;
		int numb = byte1 + (byte2 * 0x100);

		if(numb != 0) donereading = true;
		else
		{
			isetsize += 2;
			if(isetsize >= MaximumPossibleSize) donereading = true;
		}
	}

	return isetsize;
}
















