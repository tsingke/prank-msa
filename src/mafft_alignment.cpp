#include "mafft_alignment.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include "config.h"
#include <algorithm>

using namespace std;

Mafft_alignment::Mafft_alignment()
{
}

bool Mafft_alignment::test_executable()
{
    #if defined (__CYGWIN__)
    char path[200];
    int length = readlink("/proc/self/exe",path,200-1);

    string epath = string(path).substr(0,length);
    epath.replace(epath.rfind("prank"),string("prank").size(),string(""));
    mafftpath = epath;
    epath = epath+"sh.exe "+epath+"mafft -h >/dev/null 2>/dev/null";
    int status = system(epath.c_str());
    return WEXITSTATUS(status) == 1;

    # else
    int status = system("mafft -h >/dev/null 2>/dev/null");
    if(WEXITSTATUS(status) == 1)
        return true;

    char path[200];
    int length = readlink("/proc/self/exe",path,200-1);
    string epath = string(path).substr(0,length);
    epath.replace(epath.rfind("prank"),string("prank").size(),string(""));
    mafftpath = epath;
    epath = epath+"mafft -h >/dev/null 2>/dev/null";
    status = system(epath.c_str());
    return WEXITSTATUS(status) == 1;
    #endif
}

void Mafft_alignment::align_sequences(vector<string> *names,vector<string> *sequences)
{
    ofstream m_output;
    string tmp_dir = this->get_temp_dir();

    int r = rand();
    while(true)
    {

        stringstream m_name;
        m_name <<tmp_dir<<"m"<<r<<".fas";
        ifstream m_file(m_name.str().c_str());

        if(!m_file)
        {
            m_output.open( m_name.str().c_str(), (ios::out) );
            break;
        }
        r = rand();
    }

    vector<string>::iterator ni = names->begin();
    vector<string>::iterator si = sequences->begin();

    for(;ni!=names->end();ni++,si++)
    {
        m_output<<">"<<*ni<<endl<<*si<<endl;
    }
    m_output.close();

    stringstream command;
    command << mafftpath<<"mafft "+tmp_dir+"m"<<r<<".fas  2>/dev/null";

//    #if defined (__CYGWIN__)
//    char path[200];
//    int length = readlink("/proc/self/exe",path,200-1);

//    string epath = string(path).substr(0,length);
//    epath.replace(epath.rfind("prank"),string("prank").size(),string(""));
//    command << epath<<"sh.exe "<<epath<<"mafft "<<tmp_dir<<"m"<<r<<".fas 2>/dev/null";
//    # else
//    command << "mafft "+tmp_dir+"m"<<r<<".fas  2>/dev/null";
//    #endif

    FILE *fpipe;
    if ( !(fpipe = (FILE*)popen(command.str().c_str(),"r")) )
    {
        perror("Problems with mafft pipe.\nExiting.\n");
        exit(1);
    }

    names->clear();
    sequences->clear();

    // read mafft output
    string name, sequence = "";  // Initialization
    char temp[256];

    while ( fgets( temp, sizeof temp, fpipe))
    {
    	string line(temp);

        if (line[0] == '>')
        {
            line = this->remove_last_whitespaces(line);

            // If a name and a sequence were found
            if ((name != "") && (sequence != ""))
            {
                names->push_back(name);
                sequence = this->remove_whitespaces(sequence);
                transform( sequence.begin(), sequence.end(), sequence.begin(), (int(*)(int))toupper );
                sequences->push_back(sequence);
                name = "";
                sequence = "";
            }
            name = line;
            name.erase(name.begin());  // Character > deletion
        }
        else
        {
            sequence += temp;  // Sequence isolation
        }
    }

    // Addition of the last sequence in file
    if ((name != "") && (sequence != ""))
    {
        names->push_back(name);
        sequence = this->remove_whitespaces(sequence);
        transform( sequence.begin(), sequence.end(), sequence.begin(), (int(*)(int))toupper );
        sequences->push_back(sequence);
    }

    pclose(fpipe);

    this->delete_files(r);
}

void Mafft_alignment::delete_files(int r)
{

string tmp_dir = this->get_temp_dir();

stringstream m_name;
m_name <<tmp_dir<<"m"<<r<<".fas";


if ( remove( m_name.str().c_str() ) != 0 )
    perror( "Error deleting file" );
}