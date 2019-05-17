// bobob.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <tchar.h>
#include "md5.h"
#include "encode.h"
#include "decode.h"
#include "targetver.h"
#include "tinyxml.h"
#include "tinystr.h"

using namespace std;

// Globals
// Pathnames require escaped 
string METAINSTALLDIR = "C:\\Program Files\\ODL MetaTrader 4\\";
string TEMPDIR = "C:\\GAVTMP\\";

bool GetFile();
bool UnpackFile();
void CleanMetaTraderFiles();
void CleanFiles();
void CleanAllFiles();
string PackResults();
bool UploadResults();
bool CompileMq4(string mq4Arg);
string Base64Encode(string StringToEncode);
string Base64Decode(string StringToDecode);
void SleepPrintDot();

// int main(int argc, char *argv[])

int main (VOID) {
	  
    bool jobExists = FALSE;					// New job?
	while (TRUE){
          // Check for jobs
		  cout << "Checking for job..." << endl;
          jobExists = GetFile() && UnpackFile();
    
          // If a job exists: 
          //   jobExists = TRUE;
    
          // While a job exists:
          while (jobExists) {
                cout << "Job exists.. Processing" << endl;
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
    
                // Allocate memory
			    memset(&si, 0, sizeof(si));
			    memset(&pi, 0, sizeof(pi));

                //ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                //ZeroMemory(&pi, sizeof(pi));
				
				string CommandLine2Str = "\"" + METAINSTALLDIR + "terminal.exe\" \"" + METAINSTALLDIR + "thisone.ini\"";
				// Dont ask dont tell
				LPSTR szCmdline2=(LPSTR)CommandLine2Str.c_str();


                // Create child process
                if (CreateProcess(NULL,                // Use Command line
					szCmdline2,  // program to start, command line
					NULL,                                                 // don't inherit process hnadle
                    NULL,                                                 // Don't inherit thread handle
                    FALSE,                                                // disable handle inheritance
                    0,                                                    // no creation flags
                    NULL,                                                 // use parent's environment block
                    NULL,                                                 // use parent's existing directory ***
                    &si,
                    &pi)==FALSE)
                    {
                         fprintf(stderr, "Create terminal child process failed\n");
                         return -1;
                    }
                // Parent will wait for child to complete
                WaitForSingleObject(pi.hProcess, INFINITE);
                printf("Terminal Child complete\n");
       
                // Close handles (Reclaim memory? - no apparent memory leak :) )
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
       
				// Test success of terminal child
				// If successful {
				if (TRUE) {
								
					// Pack results should not fail.. if it does..
					// Clean all files and start again
					string PathToPackResultsFile;
					PathToPackResultsFile = PackResults();

					if (PathToPackResultsFile=="BAD") {
						cout << "ERROR: UPLOADME.xml failed" << endl;
					}
							
					// Send the resulting files to ZJI
					bool UploadSuccess = UploadResults();
					while (!UploadSuccess) {
						// Sleep ten minutes and try again
						SleepPrintDot();
						UploadSuccess = UploadResults();
					}
					if (UploadSuccess) {
						CleanAllFiles();
					}

					cout << "Job Finished" << endl;;

				}		// End of if terminal successfull test

				// Sleep ten minutes
				cout << "Sleeping..." << endl;
				SleepPrintDot();
       
                // Check for new Job
				cout << "Checking for new Job..." << endl;
                jobExists = GetFile() && UnpackFile();
          }
          printf("No Job exists... Sleeping\n");
          SleepPrintDot();
    }
    return EXIT_SUCCESS;
}

bool GetFile() {
     STARTUPINFO s2i;
     PROCESS_INFORMATION p2i;

     // Allocate memory
     ZeroMemory(&s2i, sizeof(s2i));
     s2i.cb = sizeof(s2i);
     ZeroMemory(&p2i, sizeof(p2i));
	 string DamnWin32Strings1 = "C:\\WINDOWS\\system32\\curl.exe -K " + TEMPDIR + "curlDownConfig.txt";
     if (!CreateProcess(NULL,                // Use Command line
	//	 TEXT("C:\\WINDOWS\\system32\\curl.exe -k -u gavine:skaterboi -s https://zji.homelinux.net/restricted_content/metatrader/getWorkUnit.php -o C:\\GAVTMP\\XMLDOWN.xml"), // program to start, command line
		 TEXT((LPSTR)DamnWin32Strings1.c_str()),
        NULL,                                                 // don't inherit process hnadle
        NULL,                                                 // Don't inherit thread handle
        FALSE,                                                // disable handle inheritance
        0,                                                    // no creation flags
        NULL,                                                 // use parent's environment block
        NULL,                                                 // use parent's existing directory ***
        &s2i,
        &p2i)) {
             fprintf(stderr, "Create Curl child process failed\n");
             return FALSE;
     }

     // Parent will wait for child to complete
     WaitForSingleObject(p2i.hProcess, INFINITE);
     
	 // Close handles (Reclaim memory? - no apparent memory leak :) )
     CloseHandle(p2i.hProcess);
     CloseHandle(p2i.hThread);
	 
	 // Test for child success...
	 // if (CHILDSUCCESS) {
     cout << "Successfully Downloaded a job..." << endl;

	 // And return true
     return TRUE;
	 // otherwise...
	 // return FALSE;
}

bool UnpackFile() {   
     // Open file. if not xml, return false;
	string ExpletiveWin32Strings = TEMPDIR + "XMLDOWN.xml";
	cout << ExpletiveWin32Strings.c_str() << endl;
     TiXmlDocument xmldoc(ExpletiveWin32Strings.c_str());
     bool loadOkay = xmldoc.LoadFile();
	const char* md5string;
	// const char* datastring;
	string datastring;
	string md5RetVal;

	 if (loadOkay) {
		 cout << "XMLDOWN.xml loaded" << endl;
         TiXmlElement *pRoot, *pMd5, *pData;
         pRoot = xmldoc.FirstChildElement( "xmltransfer" );
         if ( pRoot ) {
			 // Parse xfer
             pMd5 = pRoot->FirstChildElement("md5");
             if ( pMd5 ) {
				md5string = pMd5->GetText();
				cout << "Found embedded md5 value : " << md5string << endl;
             }
             pData = pRoot->FirstChildElement("data" );
             if ( pData ) {
                 datastring = pData->GetText();
			}
		 }
	 }

	 // Generate md5 of SUBXML.xml
	 MD5 context;
	 unsigned int len = strlen (datastring.c_str());
	 context.update   ((unsigned char*)datastring.c_str(), len);
	 context.finalize ();
	 md5RetVal = context.hex_digest();

    cout <<  "OO_MD5 = "  <<  md5RetVal << endl;
	 if (md5RetVal!= md5string) {
			cout << "MD5 Does Not match embedded value" <<endl;
			CleanAllFiles();
			return false;
	 } else {
		 cout << "MD5 values match... Continuing" << endl;
	 }

	string mq4CompileName;
	string subXmlDecoded;

	// Decode The downloaded file
	base64::decoder D;
	stringstream iss(datastring);			// String streams are sexc ;)
	stringstream oss(subXmlDecoded);
	D.decode(iss, oss);

	string itHolder, inStr8;
		getline(oss, inStr8);
	while (oss) {
		itHolder+= inStr8;
		getline(oss, inStr8);
	}
	string DamnWin32Strings6 = TEMPDIR + "demotest.xml";
	cout << DamnWin32Strings6 << endl;

	TiXmlDocument xmlAAdoc( DamnWin32Strings6.c_str() );	// This should never be written
	const char* TOGOIN = itHolder.c_str();

	// Save SUBXML to file
	/*
	std::ofstream Testing;
	Testing.open("C:\\GAVTMP\\SUBXML.txt");
	Testing << TOGOIN;
	Testing.close();
	*/
	xmlAAdoc.Parse(TOGOIN);

	if ( xmlAAdoc.Error() )
	{
		printf( "Error in %s: %s\n", xmlAAdoc.Value(), xmlAAdoc.ErrorDesc() );
	}

	//xmlAAdoc.SaveFile();			// Dont call

	string subnamestring;
	string subdatastring;
	string subfiletypestring;
	string subfileTransactionID;

	TiXmlElement *paRoot, *pSubfile, *pSubfilename, *pSubfiledata;

	paRoot = xmlAAdoc.FirstChildElement( "files" );
    if ( paRoot ) {

		subfileTransactionID= paRoot->Attribute("transaction_id");
		cout << "Transaction ID: " << subfileTransactionID << endl;
		// Parse xfer
        pSubfile = paRoot->FirstChildElement("file");
        while (pSubfile ) {
			subfiletypestring = pSubfile->Attribute("type");
			pSubfilename = pSubfile->FirstChildElement("name");
			if (pSubfilename) {
				 subnamestring = pSubfilename->GetText();
			}

			pSubfiledata = pSubfile->FirstChildElement("data");
			if (pSubfiledata) {
				 subdatastring = pSubfiledata->GetText();
			}

			// Check for emmpty tag contents
			if (subnamestring=="") {
				cout << "A decoded file is missing its name text." << endl;
			}
			if (subfiletypestring=="") {
				cout << "A decoded file is missing its type text." << endl;
			}
			if (subdatastring=="") {
				cout << "A decoded file is missing its data Text." << endl;
			}

			string cmdLineString, mq4Location;
			// Case of filetype
			if(subfiletypestring == "mq4") {

					// Open a file to write the base64 encoded subxml to
					std::ofstream mq4out;

					// String to hold installdir/experts subdir/filename
					mq4Location = METAINSTALLDIR + "experts\\" + subnamestring;
					mq4out.open(mq4Location.c_str());

					// Decode
					base64::decoder Dmq4;
					stringstream issmq4(subdatastring);
					Dmq4.decode(issmq4, mq4out);
					mq4out.close();
					cout << "Base64 Decoded File type: " << subfiletypestring << "\nFile name: " << subnamestring << endl;
					mq4CompileName = subnamestring;
			}
			// This will no longer be called as the .ex4 is no longer provided in the source file
			if(subfiletypestring == "ex4") {
					mq4Location = METAINSTALLDIR + "experts\\" + subnamestring;
					// Open a file to write the base64 encoded subxml to
					std::ofstream ex4out;
					ex4out.open(mq4Location.c_str());
					
					// Decode
					base64::decoder Dex4;
					stringstream issex4(subdatastring);
					Dex4.decode(issex4, ex4out);
					ex4out.close();	
					cout << "Base64 Decoded File type: " << subfiletypestring << "\nFile name: " << subnamestring << endl;
			}
			if(subfiletypestring == "set") {
					// String to hold .set subdir
					string setLocation = METAINSTALLDIR + "tester\\" + subnamestring;
					// Open a file to write the base64 encoded subxml to
					std::ofstream setout;
					setout.open(setLocation.c_str());
					base64::decoder Dset;
					stringstream issset(subdatastring);
					Dset.decode(issset, setout);
					setout.close();
					cout << "Base64 Decoded File type: " << subfiletypestring << "\nFile name: " << subnamestring << endl;
			}
			if(subfiletypestring == "ini") {
					// Open a file to write the base64 encoded subxml to
					std::ofstream iniout;
					string text2str2c = METAINSTALLDIR + "thisone.ini";
					iniout.open(text2str2c.c_str());
					base64::decoder Dini;
					stringstream issini(subdatastring);
					Dini.decode(issini, iniout);
					iniout.close();
					cout << "Base64 Decoded File type: " << subfiletypestring << "\nFile name: " << subnamestring << endl;
			}
			pSubfile = pSubfile->NextSiblingElement("file");
		}
		cout << "Required Files Generated" << endl;
	}

	// Test for successfull mq4 compile
	// if (!CompileMq4(mq4CompileName)) {
	//		return false;
	// }

	// Compile the mq4
	CompileMq4(mq4CompileName);
	
	 // Write the TransactionID to a FILE for locking
	 std::ofstream TransactionIDLock;
	 string DamnWin32Strings2 = TEMPDIR + "FILELOCK.txt";
	 TransactionIDLock.open(DamnWin32Strings2.c_str());
	 TransactionIDLock << subfileTransactionID;
	 TransactionIDLock.close();

      return TRUE;
}

void CleanMetaTraderFiles() {
	
	string subfileTransactionID, htmString, gifString;	// String to hold the transaction ID
	// Need to get this string subfileTransactionID from unpackfile()
	std::ifstream TransactionIDLock;
	string DamnWin32Strings7 = TEMPDIR + "FILELOCK.txt";
	TransactionIDLock.open(DamnWin32Strings7.c_str());
	if (!TransactionIDLock.is_open()) {
		cout << "Failed to Open FILELOCK.txt for reading" << endl;
	}
	TransactionIDLock >> subfileTransactionID;
	TransactionIDLock.close();

	htmString = METAINSTALLDIR + subfileTransactionID + ".htm";
	gifString = METAINSTALLDIR + subfileTransactionID + ".gif";

 	if (!DeleteFile(htmString.c_str())) {
		cout << "Failed to delete " << htmString << endl;
	} else {
		cout << "Deleted " << htmString << endl;
	}
	if (!DeleteFile(gifString.c_str())) {
		cout << "Failed to delete " << gifString << endl;
	} else {
		cout << "Deleted " << gifString << endl;
	}

	// Open a ini file to read the names of mq4 and set files
	std::ifstream iniIn;
	string text2str2ca = METAINSTALLDIR + "thisone.ini";
	iniIn.open(text2str2ca.c_str());
	cout << text2str2ca << endl;
	if (!iniIn.is_open()) {
	cout << "Failed to Open thisone.ini report for reading" << endl;
	}

	// Parse the file to determine Expert and set names
	// Read in a line. test for "TestExpert= EXPERT NAME HERE"
	// or "TestExpertParameters= SET NAME HERE"
	string testLine, TestExpertName,TestExpertParametersName;
	string searchTestExpert = "TestExpert=";
	string searchTestExpertParameters = "TestExpertParameters=";
	size_t pos, pos2;
	char* TempChar;
	iniIn.getline(TempChar, 256);
	cout << TempChar <<endl;
	while (!iniIn.eof()) {
		testLine = TempChar;
		pos = testLine.find(searchTestExpert);
		pos2 = testLine.find(searchTestExpertParameters);
		if (pos!=string::npos) {

			// Found name of TestExpert
			TestExpertName = testLine.substr(pos + searchTestExpert.length());
			string DamnWin32Strings7a = METAINSTALLDIR + "experts\\" + TestExpertName + ".ex4";
			if (!DeleteFile(TEXT(DamnWin32Strings7a.c_str()))) {
			cout << "Failed to delete " << TestExpertName << ".ex4 file" << endl;
			} else {
				cout << "Deleted " << TestExpertName << ".ex4" << endl;
			}
			string DamnWin32Strings7b = METAINSTALLDIR + "experts\\" + TestExpertName + ".mq4";
			if (!DeleteFile(TEXT(DamnWin32Strings7b.c_str()))) {
			cout << "Failed to delete " << TestExpertName << ".mq4 file" << endl;
			} else {
				cout << "Deleted " << TestExpertName << ".mq4" << endl;
			}
		}
		if (pos2!=string::npos) {

			// Found name of TestExpertParameters
			TestExpertParametersName = testLine.substr(pos2 + searchTestExpertParameters.length());
			string DamnWin32Strings7c = METAINSTALLDIR + "tester\\" + TestExpertParametersName;
			if (!DeleteFile(TEXT(DamnWin32Strings7c.c_str()))) {
			cout << "Failed to delete " << TestExpertParametersName << endl;
			} else {
				cout << "Deleted " << TestExpertParametersName << endl;
			}
		}
		iniIn.getline(TempChar, 256);
		testLine = TempChar;
	}

	// Close the file handle
	iniIn.close();
	
	// Delete terminal's ini file
	string DamnWin32Strings = METAINSTALLDIR + "thisone.ini";
	if (!DeleteFile(TEXT(DamnWin32Strings.c_str()))) {
		cout << "Failed to delete thisone.ini" << endl;
	} else {
		cout << "Deleted thisone.ini" << endl;
	}
}

void CleanFiles() {
     // Not strictly neccesary as curl clobbers the old file

	string DamnWin32Strings8 = TEMPDIR + "XMLDOWN.xml";
	if (!DeleteFile(TEXT(DamnWin32Strings8.c_str()))) {
        cout << "Failed to delete XMLDOWN.xml" << endl;
	} else {
		cout << "Deleted XMLDOWN.xml" << endl;
	}
}

void CleanAllFiles() {
	cout << "Cleaning All Files..." << endl;

     CleanFiles();
	 CleanMetaTraderFiles();
	 string DamnWin32Strings3 = TEMPDIR + "curloutput.txt";
	 string DamnWin32Strings4 = TEMPDIR + "FILELOCK.txt";
	 string DamnWin32Strings5 = TEMPDIR + "UPLOADME.xml";
 	if (!DeleteFile(TEXT(DamnWin32Strings3.c_str()))) {
        cout << "Failed to delete curloutput.txt" << endl;
	} else {
		cout << "Deleted curloutput.txt" << endl;
	}
 	if (!DeleteFile(TEXT(DamnWin32Strings4.c_str()))) {
        cout << "Failed to delete FILELOCK.txt" << endl;
	} else {
		cout << "Deleted FILELOCK.txt" << endl;
	}
	if (!DeleteFile(TEXT(DamnWin32Strings5.c_str()))) {
        cout << "Failed to delete UPLOADME.xml" << endl;
	} else {
		cout << "Deleted UPLOADME.xml" << endl;
	}
}

string PackResults() {
	// Test for the existence of result files

	string subfileTransactionID;	// String to hold the transaction ID
	
	// Need to get this string subfileTransactionID from unpackfile()
	std::ifstream TransactionIDLock;
	string DamnWin32Strings5 = TEMPDIR + "FILELOCK.txt";
	TransactionIDLock.open(DamnWin32Strings5.c_str());
	if (!TransactionIDLock.is_open()) {
		cout << "Failed to Open FILELOCK.txt for reading" << endl;
		return("BAD");
	}
	TransactionIDLock >> subfileTransactionID;
	TransactionIDLock.close();

	// Get the Report files in
	string  rptGifIn, rptHtmlIn, ReportHtmlInString, b64_ReportGifInString, b64_ReportHtmlInString;

	// Get the Gif Report file in
	ifstream reportGifIn((METAINSTALLDIR + subfileTransactionID + ".gif").c_str(),ios::binary);
	//reportGifIn.open(("C:\\Program Files\\ODL MetaTrader 4\\" + subfileTransactionID + ".gif").c_str());
	if (!reportGifIn.is_open()) {
		cout << "Failed to Open Gif report for reading" << endl;
		return("BAD");
	}

	std::stringstream buffer;
	buffer << reportGifIn.rdbuf();
	std::string ReportGifInString(buffer.str());
	reportGifIn.close();

	// Get the Html Report file in
	ifstream reportHtmlIn;
	reportHtmlIn.open((METAINSTALLDIR + subfileTransactionID + ".htm").c_str());
	if (!reportHtmlIn.is_open()) {
		cout << "Failed to Open Html report for reading" << endl;
		return("BAD");
	}
	getline(reportHtmlIn, rptHtmlIn);
	while (reportHtmlIn) {
		ReportHtmlInString+= rptHtmlIn;
		getline(reportHtmlIn, rptHtmlIn);
	}
	reportHtmlIn.close();
	
	// Encode the Gif file
	stringstream issG(ReportGifInString);	
	stringstream ossG(b64_ReportGifInString);
	base64::encoder E1;
	E1.encode(issG, ossG);
	string itGifHolder, inStrG8;	// More strings
	getline(ossG, inStrG8);
	while (ossG) {
		itGifHolder+= inStrG8;		// itGifHolder now holds the uuencdoed Gif file ready to
		getline(ossG, inStrG8);		// Place into the UPLOADME.xml
	}
	
	// Encode the Html file
	stringstream issH(ReportHtmlInString);	
	stringstream ossH(b64_ReportHtmlInString);
	base64::encoder E2;
	E2.encode(issH, ossH);
	string itHtmlHolder, inStrH8;	// More strings
	getline(ossH, inStrH8);
	while (ossH) {
		itHtmlHolder+= inStrH8;		// itHtmlHolder now holds the Html uuencdoed file ready to
		getline(ossH, inStrH8);		// Place into the UPLOADME.xml
	}

	// Pack Report results into an xml
	TiXmlDocument UpSubDoc;
	TiXmlDeclaration * UpSubDecl = new TiXmlDeclaration( "1.0", "UTF-8", "yes" );
	UpSubDoc.LinkEndChild( UpSubDecl );

	TiXmlElement * resultsElement = new TiXmlElement( "results" );
	UpSubDoc.LinkEndChild( resultsElement );

	resultsElement->SetAttribute("transaction_id", subfileTransactionID.c_str());

	// Create the res_html node
	TiXmlElement * res_htmlElement = new TiXmlElement( "res_html" );
	resultsElement->LinkEndChild( res_htmlElement );

	// Dump the Encoded Html string into the xml text
	TiXmlText * resultsTexthtm = new TiXmlText( itHtmlHolder.c_str() );
	res_htmlElement->LinkEndChild( resultsTexthtm );

	// Create the res_image node
	TiXmlElement * res_imageElement = new TiXmlElement( "res_image" );
	resultsElement->LinkEndChild( res_imageElement );

	// Dump the gif string into the xml text
	TiXmlText * resultsTextgif = new TiXmlText( itGifHolder.c_str() );
	res_imageElement->LinkEndChild( resultsTextgif );

	// Print the UpSubDoc to an xml string
	string strUpSubDocXmlB64_md5;	// String to hold the md5 of the data contents of UPLOADME.xml
	string strUpSubDocXml, strUpSubDocXmlB64;
	strUpSubDocXml << UpSubDoc;

	// Base64 encode the string
	strUpSubDocXmlB64 = Base64Encode(strUpSubDocXml);

	// Create the final xml file
	TiXmlDocument finDoc;
	TiXmlDeclaration * findecl = new TiXmlDeclaration( "1.0", "UTF-8", "yes" );
	finDoc.LinkEndChild( findecl );

	TiXmlElement * xmltransferElement = new TiXmlElement( "xmltransfer" );
	finDoc.LinkEndChild( xmltransferElement );

	TiXmlElement * md5Element = new TiXmlElement( "md5" );
	xmltransferElement->LinkEndChild( md5Element );

	// Generate the MD5 for the UPLOADME.xml file
	MD5 contextFin;
	unsigned int len = strlen (strUpSubDocXmlB64.c_str());
	contextFin.update   ((unsigned char*)strUpSubDocXmlB64.c_str(), len);
	contextFin.finalize ();
	strUpSubDocXmlB64_md5 = contextFin.hex_digest();
	cout <<  "UPLOADME.xml MD5 Contents: "  <<  strUpSubDocXmlB64_md5 << endl;

	// Insert MD5 into UPLOADME.xml contents
	TiXmlText * md5Text = new TiXmlText( strUpSubDocXmlB64_md5.c_str() );
	md5Element->LinkEndChild( md5Text );

	// Insert encoded data into UPLOADME.xml contents
	TiXmlElement * dataElement = new TiXmlElement( "data" );
	xmltransferElement->LinkEndChild( dataElement );
	TiXmlText * dataText = new TiXmlText(strUpSubDocXmlB64.c_str());
	dataElement->LinkEndChild( dataText );

	// Save the resulting xml file..
	// If (SUCCESS==
	finDoc.SaveFile( "C:\\GAVTMP\\UPLOADME.xml" );

	// ) {
	// Return the PAth and filename of the Results file
	return "C:\\GAVTMP\\UPLOADME.xml";
	// } else a problem has occured
	// return("BAD");
}

bool UploadResults() {

	// upload with curl
	printf("Uploading... \n");
	STARTUPINFO s5i;
	PROCESS_INFORMATION p5i;
    
    // Allocate memory
    memset(&s5i, 0, sizeof(s5i));
    memset(&p5i, 0, sizeof(p5i));

    //ZeroMemory(&s5i, sizeof(s5i));
    s5i.cb = sizeof(s5i);
    //ZeroMemory(&p5i, sizeof(p5i));

    // Create child process
    if (CreateProcess(NULL,                // Use Command line
		
		//"C:\\WINDOWS\\system32\\curl.exe -k -u gavine:skaterboi -s -F resultfile=C:\\GAVTMP\\UPLOADME.xml -F completed=1 -F MAX_FILE_SIZE=10000000 -F ClickHere=ClickHere -F action=/restricted_content/metatrader/return_results.php https://zji.homelinux.net/restricted_content/metatrader/return_results.php",
		TEXT("C:\\WINDOWS\\system32\\curl.exe -K C:\\GAVTMP\\curlUpConfig.txt"), 		  // program to start, command line
		NULL,                                                 // don't inherit process hnadle
        NULL,                                                 // Don't inherit thread handle
        FALSE,                                                // disable handle inheritance
        0,                                                    // no creation flags
        NULL,                                                 // use parent's environment block
        NULL,                                                 // use parent's existing directory ***
        &s5i,
        &p5i)==FALSE)
        {
             fprintf(stderr, "Create Upload child process failed\n");
             return false;
        }

    // Parent will wait for child to complete
    WaitForSingleObject(p5i.hProcess, INFINITE);
    cout << "Upload complete. Checking Authenticity... " << endl;

    // Close handles (Reclaim memory? - no apparent memory leak :) )
    CloseHandle(p5i.hProcess);
    CloseHandle(p5i.hThread);

	//Test exit status of curl and return Success status
	ifstream curlResults;
	curlResults.open("C:\\GAVTMP\\curloutput.txt");
	string strCurlResult, itCurlResult;
	getline(curlResults, itCurlResult);
	while (curlResults) {
		strCurlResult+= itCurlResult;
		getline(curlResults, itCurlResult);
	}
	curlResults.close();

	// If transaction_id=TransactionIDVAlue is in the string strCurlResult
	//	{return true;} else { cout << "File Upload Failed, Trying again soon" <<endl; } return false;
	ifstream searchFor;
	searchFor.open("C:\\GAVTMP\\FILELOCK.txt");
	string stringToSearchFor;
	getline(searchFor, stringToSearchFor);
	searchFor.close();

	size_t found;
	found=strCurlResult.find(stringToSearchFor);
	if (found!=string::npos) {
		cout << "Authentic Upload." << endl;
		return true;
	}
	cout << "Upload Failed Authenticity check. Requeing Upload..." <<endl;
	return false;
}

bool CompileMq4(string mq4Arg) {

	// Get rid of mqlcache.dat
	string DamnWin32Strings = METAINSTALLDIR + "experts\\mqlcache.dat";
 	if (!DeleteFile(TEXT(DamnWin32Strings.c_str()))) {
        cout << "Failed to delete mqlcache.dat" << endl;
	} else {
		cout << "Deleted mqlcache.dat" << endl;
	}

	// Run "C:\\Program Files\\ODL MetaTrader 4\\metalang -q C:\\Program Files\\ODL MetaTrader 4\\experts\\ FILE.mq4
	cout << "Compiling: " << mq4Arg << endl;
	string CommandLine1Str = "\"" + METAINSTALLDIR + "metalang.exe\" -q \"" + METAINSTALLDIR + "experts\\" + mq4Arg + "\"";

	// Convert string into the format win32 requires *** FIX THIS
	LPSTR szCmdline1=(LPSTR)CommandLine1Str.c_str();

	STARTUPINFO s6i;
	PROCESS_INFORMATION p6i;
    
    // Allocate memory
    memset(&s6i, 0, sizeof(s6i));
    memset(&p6i, 0, sizeof(p6i));

    //ZeroMemory(&s6i, sizeof(s6i));
    s6i.cb = sizeof(s6i);
    //ZeroMemory(&p6i, sizeof(p6i));

    // Create child process
    if (CreateProcess(NULL,									  // Use Command line
		szCmdline1,
		NULL,                                                 // don't inherit process hnadle
        NULL,                                                 // Don't inherit thread handle
        FALSE,                                                // disable handle inheritance
        0,                                                    // no creation flags
        NULL,                                                 // use parent's environment block
        NULL,                                                 // use parent's existing directory ***
        &s6i,
        &p6i)==FALSE)
        {
             fprintf(stderr, "Compile mq4 child process failed\n");
             return false;
        }

    // Parent will wait for child to complete
    WaitForSingleObject(p6i.hProcess, INFINITE);
	
	// Close handles (Reclaim memory? - no apparent memory leak :) )
    CloseHandle(p6i.hProcess);
    CloseHandle(p6i.hThread);

	// Test successfull exit of process
	// if (EXIT_SUCCESS) {
    cout << "Mq4 Compiled" << endl;

	// And return true
	return true;

	// } other wise it failed
	// cout << "Mq4 FAILED to compile" << endl;
	// Should probably reget the job.
	// return false;
}

string Base64Encode(string StringToEncode) {
	// Encode the string
	string GenericInString = StringToEncode;
	string Genericb64_InString;
	stringstream issGeneric(GenericInString);	
	stringstream ossGeneric(Genericb64_InString);
	base64::encoder EG;
	EG.encode(issGeneric, ossGeneric);
	string GenericHolder, itGenericStr;	// More strings
	getline(ossGeneric, itGenericStr);
	while (ossGeneric) {
		GenericHolder+= itGenericStr;		// itGifHolder now holds the uuencdoed Gif file ready to
		getline(ossGeneric, itGenericStr);		// Place into the UPLOADME.xml
	}
	return GenericHolder;
}

string Base64Decode(string StringToDecode) {
	//Decode the string
	string Genericb64_inInString = StringToDecode;
	string GenericOutString;
	stringstream issGeneric(Genericb64_inInString);
	stringstream ossGeneric(GenericOutString);
	base64::decoder DG;
	DG.decode(issGeneric, ossGeneric);
	string GenericHolder, inGenericStr;
		getline(ossGeneric, inGenericStr);
	while (ossGeneric) {
		GenericHolder+= inGenericStr;
		getline(ossGeneric, inGenericStr);
	}
	return GenericHolder;
}
void SleepPrintDot() {
	int minsSleep = 10;				// Default sleep 10 Minutes between job requests    
    int SleepDotArgs = 1000*20;		// In ticks of 1 millisecond 20 seconds total
//	int SleepArgs = 3 * minsSleep;	// 30 iterations = 10 minutes
	int SleepArgs = 3;				// 3 iterations = 60 seconds
	int i;
	for (i=0; i< SleepArgs; i++) {
		Sleep(SleepDotArgs);
		cout << ". ";
	}
}