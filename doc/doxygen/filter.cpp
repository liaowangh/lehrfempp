// Author: Huang Liaowang
// Date: January 2020
// This is part of the LehrFEM++ code suite

/*
This script is to replaces all the @lref{<label>} statements in a C++ files with a label-specific
string and the corresponding number from .aux file. Here we assume the line corresponding to this
 label in .aux file looks like

\newlabel{<label>@cref}{{[<Label>][x][xxx]<number>}{[x][xx][x]xxx}}

We would like to replace @lref{<label>} with <Label> <number>, which means that label is
extracted from the first square brackets and the number comes from the content after two
brackets of <Label>.
*/

/*
The ultimate goal is to have references to LaTeX documents in documentation generated
by Doxygen, and this script is to do the preprocessing before the Doxygen parse the source
file.

To use this filter, specifiy
    INPUT_FILTER  = "g++ -std=c++11" "filter.cpp -o filter &&./filter "
and put this script and "NCSE19refs.aux" together with doxygen configuration file.
*/

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <streambuf>
#include <regex>
#include <unordered_map>

using namespace std;

class Filter{
private:
    unordered_map<string, string> _label_dict;
    unordered_map<string, string> _aux_dict;
    string _aux_file;
    string _file;
public:
    Filter(string file, string aux_file): _file(file), _aux_file(aux_file){
        _label_dict = {
            {"equation", "Equation"},
            {"par", "Paragraph"},
            {"chapter", "Chapter"},
            {"sec", "Section"},
            {"subsection", "Subsection"},
            {"figure", "Figure"},
            {"code", "Code"},
            {"remark", "Remark"}
        };
    }
    
    string label_number(string ref);
    void aux_dict();
    void replace_lref();
};


string Filter::label_number(string ref){
    /*
    :param ref: a line in .aux file, e.g.
        '\newlabel{eq:vec@cref}{{[equation][5][1,2,3]1.2.3.5}{[1][44][]44}}'
    :param label_dict: a dictionary, e.g. convert 'equation' to 'Equation'
    :return: label+number, e.g. Equation 1.2.3.4
    */
    smatch match;
    regex_search(ref, match, regex("\\{\\{(.*?)\\}"));
    string tmp = match[1];  // [equation][5][1,2,3]1.2.3.5, get the content between  {{ ... }
    
    regex_search(tmp, match, regex("\\[(.*?)\\]"));
    string label = match[1];  // extract label from first square brackets
    while(regex_search(tmp, match, regex("\\[(.*?)\\]")))
          tmp = match.suffix();
    string number = tmp;  // number is last part of the string
    
    return (_label_dict.count(label) ? _label_dict[label] : label) + " " + number;
}

void Filter::aux_dict(){
    /*
    parse the .aux file and store in a map.
    The line in .aux file we are interested is assumed to look like:
    \newlabel{<label>@cref}{{[<Label>][x][xxx]<number>}{[x][xx][x]xxx}}
    And we search for "@cref" to find the line.

    :param aux_file: the .aux file
    :return: a dictionary maps all the reference to label + number
    */
    ifstream infile(_aux_file, ios::binary);  // open the aux file
    string line;
    smatch matches;
    regex label_pattern("\\{(.*?)@cref\\}");
    while(getline(infile, line)){  // read line by line
        regex_search(line, matches, label_pattern);
        if(matches.ready())
            _aux_dict[matches.str(1)] = label_number(line);
    }
}

void Filter::replace_lref(){
    ifstream t(_file, ios::binary);  // open the file
    string file_string((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());  // read into a string
    string result;
    regex lref("@lref\\{(.*?)\\}");
    smatch match;
    while(regex_search(file_string, match, lref)){
        result += match.prefix().str();
        if(!_aux_dict.count(match[1])){
            cout << "Cannot find a label for " << match[1] << endl;
            result += match[0];
        }
        else
            result += _aux_dict[match[1]];
        file_string = match.suffix().str();
    }
    result += file_string;
    cout << result;
}

int main(int argc, char* argv[]){
    // takes a file as input and print the content after parsing.
    // the output is cout which can be understood natively by doxygen.
  
    string aux_file, file;
    if(argc == 2){
        aux_file = "NPDEFLrefs.aux";
        file = argv[1];
    } else{
        aux_file =argv[1];
        file = argv[2];
    }

    
    Filter f(file, aux_file);
    f.aux_dict();
    f.replace_lref();
    
    return 0;
}