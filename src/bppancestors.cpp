/***************************************************************************
 *   Copyright (C) 2013 by Ari Loytynoja   *
 *   ari@ebi.ac.uk   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include "bppancestors.h"
#include "readfile.h"
#include "readnewick.h"

#if defined (__APPLE__)
#include <mach-o/dyld.h>
#endif

using namespace std;

BppAncestors::BppAncestors()
{
}

bool BppAncestors::testExecutable()
{
    #if defined (__CYGWIN__)
    char path[200];
    int length = readlink("/proc/self/exe",path,200-1);

    string epath = string(path).substr(0,length);
    epath.replace(epath.rfind("prank"),string("prank").size(),string(""));
    bppdistpath = epath;
    epath = epath+"bppancestor >/dev/null 2>/dev/null";
    int status = system(epath.c_str());

    return WEXITSTATUS(status) == 0;

    # else
    int status = system("bppancestor >/dev/null 2>/dev/null");

    if(WEXITSTATUS(status) == 0)
        return true;

    char path[200];
    string epath;

    #if defined (__APPLE__)
    uint32_t size = sizeof(path);
    _NSGetExecutablePath(path, &size);
    epath = string(path);
    epath.replace(epath.rfind("prank"),string("prank").size(),string(""));
    //epath = "DYLD_LIBRARY_PATH="+epath+" "+epath;

    #else
    int length = readlink("/proc/self/exe",path,200-1);
    epath = string(path).substr(0,length);
    epath.replace(epath.rfind("prank"),string("prank").size(),string(""));

    #endif

    bppdistpath = epath;
    epath = epath+"bppancestor >/dev/null 2>/dev/null";
    status = system(epath.c_str());

    return WEXITSTATUS(status) == 0;

    #endif
}

void BppAncestors::inferAncestors(AncestralNode *root,map<string,string> *aseqs,string *atree,bool isDna)
{

    string tmp_dir = this->get_temp_dir();

    stringstream f_name;
    stringstream t_name;
    stringstream o_name;
    stringstream m_name;

    int r = rand();
    while(true)
    {

        f_name <<tmp_dir<<"f"<<r<<".fas";
        ifstream f_file(f_name.str().c_str());

        t_name <<tmp_dir<<"t"<<r<<".tre";
        ifstream t_file(t_name.str().c_str());

        o_name <<tmp_dir<<"o"<<r<<".fas";
        ifstream o_file(t_name.str().c_str());

        m_name <<tmp_dir<<"m"<<r<<".tre";
        ifstream m_file(t_name.str().c_str());

        if(!f_file && !t_file && !o_file && !m_file)
        {
            break;
        }
        r = rand();
    }



    ////////////

    int l = root->getSequence()->length();

    vector<string> names;
    vector<string> sequences;

    root->getTerminalNames(&names);
    for (int i=0; i<names.size(); i++)
    {
        sequences.push_back(string(""));
    }

    vector<string> col;

    bool tmpDOTS = DOTS;
    DOTS = false;
    for (int i=0; i<l; i++)
    {
        col.clear();
        root->getCharactersAt(&col,i);
        vector<string>::iterator cb = col.begin();
        vector<string>::iterator ce = col.end();
        vector<string>::iterator si = sequences.begin();

        for (; cb!=ce; cb++,si++)
        {
            *si+=*cb;
        }
    }
    DOTS = tmpDOTS;

    vector<string>::iterator si = sequences.begin();
    vector<string>::iterator ni = names.begin();

    ofstream f_output;
    f_output.open( f_name.str().c_str(), (ios::out) );
    for(;si!=sequences.end();si++,ni++)
        f_output<<">"<<*ni<<"\n"<<*si<<"\n";
    f_output.close();


    string tree = "";

    int nodeNum = root->getTerminalNodeNumber();
    root->getNHXBrl(&tree,&nodeNum);
    stringstream tag;
    tag << root->getNodeName();
    char b,e; int num;
    tag >> b >> num >> e;
    tag.clear();
    tag.str("");
    tag << num;
    tree += "[&&NHX:ND="+tag.str()+"];";

    ofstream t_output;
    t_output.open( t_name.str().c_str(), (ios::out) );
    t_output<<tree<<endl;
    t_output.close();


    stringstream command;
    command << bppdistpath<<"bppancestor input.sequence.file="<<f_name.str()<<" input.sequence.format=Fasta input.sequence.sites_to_use=all input.tree.file="<<t_name.str()<<
            " input.tree.format=NHX input.sequence.max_gap_allowed=100% initFreqs=observed output.sequence.file="<<o_name.str()<<" output.sequence.format=Fasta";
    if(!isDna)
        command << " alphabet=Protein model=WAG01";
    else
    {
        if(CODON)
            command << " alphabet=Codon\\(letter=DNA,type=Standard\\) model=YN98\\(kappa=2,omega=0.5\\)";
        else
            command << " alphabet=DNA model=HKY85";
    }

//    cout<<command.str()<<endl;


    FILE *fpipe;
    char line[256];

    if ( !(fpipe = (FILE*)popen(command.str().c_str(),"r")) )
    {
        cout<<"Problems with bppancestor pipe.\nExiting.\n";
        exit(1);
    }
    while ( fgets( line, sizeof line, fpipe))
    {
        if(NOISE>1)
            cout<<"BppAncestor: "+string(line);
    }
    pclose(fpipe);

    /*
    // This is not needed with the fixed bppancestor
    //
    command << " output.tree_ids.file="<<m_name.str();

    if ( !(fpipe = (FILE*)popen(command.str().c_str(),"r")) )
    {
        cout<<"Problems with bppancestor pipe.\nExiting.\n";
        exit(1);
    }
    while ( fgets( line, sizeof line, fpipe))
    {
        if(NOISE>1)
            cout<<"BppAncestor: "+string(line);
    }
    pclose(fpipe);
    */

    ReadFile rf;
    rf.readFile(o_name.str().c_str());
    vector<string> s = rf.getSeqs();
    vector<string> n = rf.getNames();

    for(int i=0;i<n.size();i++)
        aseqs->insert(aseqs->begin(),pair<string,string>("#"+n.at(i)+"#",s.at(i)));

    /*
    // This is not needed with the fixed bppancestor
    //
    ReadNewick rn;
    *atree = rn.readFile(m_name.str().c_str());
    */

    this->delete_files(r);

}

void BppAncestors::delete_files(int r)
{

    string tmp_dir = this->get_temp_dir();

    stringstream t_name;
    t_name <<tmp_dir<<"t"<<r<<".tre";

    stringstream f_name;
    f_name <<tmp_dir<<"f"<<r<<".fas";

    stringstream o_name;
    o_name <<tmp_dir<<"o"<<r<<".fas";

//    stringstream m_name;
//    m_name <<tmp_dir<<"m"<<r<<".tre";

    if ( remove( t_name.str().c_str() ) != 0 )
        perror( "Error deleting file" );
    if ( remove( f_name.str().c_str() ) != 0 )
        perror( "Error deleting file");
    if ( remove( o_name.str().c_str() ) != 0 )
        perror( "Error deleting file");
//    if ( remove( m_name.str().c_str() ) != 0 )
//        perror( "Error deleting file");

}
