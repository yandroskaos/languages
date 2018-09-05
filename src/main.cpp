#include <stdlib.h>
#include <iostream>
#include <fstream>
using namespace std;

#include "EBNF.h"

class PrintVisitor : public TreeVisitor
{
	ostream& out;
	bool showPosition;
public:
	PrintVisitor(ostream& _stream, bool _showPosition);
	virtual bool Visit(STNode* _node, unsigned int _level);
};

string GetPath(const string& _fullFileName);
string GetName(const string& _fullFileName);
string Translate(char _c);

void ParserFailure		 (const Result& _r, Stream* _s);
void SemanticsFailure	 (const Result& _r);
void CodeGeneratorFailure(const Result& _r);

int main(int argc, char* argv[])
{
	//Check parameters
	if(argc != 2 && argc != 3)
	{
		cout << "Use: " << argv[0] << " <EBNF file> [-p]" << endl;
		cout << "\t<EBNF file> = file with language description" << endl;
		cout << "\t-p = Shows position in the stream of the AST nodes in the AST Tree output" << endl;
		return 0;
	}

	//Obtain file to parse and whether to show node's position or not
	char* fileName = argv[1];
	bool showPosition = (argc == 3) && (argv[2] == string("-p"));

	//char* fileName = "../test.ebnf";
	//bool showPosition = true;

	//Get stream to read the file
	Stream* fs = FileStream(fileName);
	if(!fs)
	{
		cout << "Unable to open: " << fileName << endl;
		return 0;
	}

	//Get Parser for EBNF files and do parsing
	Parser* ebnf_parser = EBNF_Parser();
	STNode* ebnf_tree = 0;
	Result r = ebnf_parser->Parse(fs, ebnf_tree);
	if(!r)
	{
		ParserFailure(r, fs);
		return 0;
	}

	//Get Semantics and check
	Semantics* ebnf_semantics = EBNF_Semantics();
	Result s = ebnf_semantics->Check(ebnf_tree);
	if(!s)
	{
		SemanticsFailure(s);
		delete ebnf_tree;
		return 0;
	}

	//Generate test file with obtained AST tree
	string stFileName = GetPath(fileName) + GetName(fileName) + ".st";
	PreWalk(ebnf_tree, new PrintVisitor(ofstream(stFileName), showPosition));
	cout << "ST  generated => " << stFileName.c_str() << endl;

	CodeGenerator* ebnf_code_generator = EBNF_CodeGenerator();
	Result t = ebnf_code_generator->Generate(ebnf_tree, GetPath(fileName));
	if(!t)
	{
		CodeGeneratorFailure(t);
		delete ebnf_tree;
		return 0;
	}

	cout << "CODE generated => " << (GetPath(fileName) + ebnf_tree->data + "(.h & .cpp)").c_str() << endl;
	cout << "Success!" << endl;
	return 0;
}

PrintVisitor::PrintVisitor(ostream& _stream, bool _showPosition)
	: out(_stream), showPosition(_showPosition)
{
}

bool PrintVisitor::Visit(STNode* _node, unsigned int _level)
{
	for(unsigned int i = 0; i < _level; i++)
		out << "\t";

	out << _node->data.c_str();

	if(showPosition)
		out << "(" << _node->where.row << ", " << _node->where.column << ")";

	out << endl;

	return true;
}

string GetPath(const string& _fullFileName)
{
	char drive[_MAX_DRIVE];
	char path[_MAX_DIR];
	if(_splitpath_s(_fullFileName.c_str(), drive, _MAX_DRIVE, path, _MAX_DIR, NULL, 0, NULL, 0) == 0)
	{
		return string(drive) + string(path);
	}

	return "";
}

string GetName(const string& _fullFileName)
{
	char name[_MAX_FNAME];
	if(_splitpath_s(_fullFileName.c_str(), NULL, 0, NULL, 0, name, _MAX_FNAME, NULL, 0) == 0)
	{
		return name;
	}

	return "";
}

string Translate(char _c)
{
	if(_c == '\n')
		return "\\n";

	if(_c == '\t')
		return "\\t";

	return string(1, _c);
}

void ParserFailure(const Result& _r, Stream* _s)
{
	cout << "Failure" << endl;

	//Put stream pointing to where the error is located 
	_s->Goto(_r.fail.where);

	cout << "At (" << _r.fail.where.row << ", " << _r.fail.where.column << ") ";
		
	//We found this character...
	cout << "Found: [" << Translate(_s->Get()) << "]" << endl; 
		
	//But we were expecting one of those...
	for(unsigned int i = 0; i < _r.fail.expected.size(); i++)
	{
		cout << "\tExpected: [" << _r.fail.expected[i] << "]" << endl; 
	}
}

void SemanticsFailure(const Result& _r)
{
	cout << "Failure" << endl;

	cout << "At (" << _r.fail.where.row << ", " << _r.fail.where.column << ")" << endl;
		
	for(unsigned int i = 0; i < _r.fail.expected.size(); i++)
	{
		cout << "\tError: [" << _r.fail.expected[i] << "]" << endl; 
	}
}

void CodeGeneratorFailure(const Result& _r)
{
	cout << "Failure" << endl;

	for(unsigned int i = 0; i < _r.fail.expected.size(); i++)
	{
		cout << "\tError: [" << _r.fail.expected[i] << "]" << endl; 
	}
}
