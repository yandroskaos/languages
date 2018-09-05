#include "EBNF.h"

#include <algorithm>

#define _SQ Sequence
#define _OR Choice
#define _ST Star
#define _PL Plus
#define _OP Optional
#define _R(X) Reference(&X)

class EBNFParser : public Parser
{
	//Sets
	Set Alpha;
	Set DecimalDigit;
	Set AlphaDigit;
	Set Whitespaces;

	//Ignorable parsers
	Parser* Separator;
	Parser* Comment;
	Parser* to_ignore;

	//Lexical parsers
	Parser* CteString;
	Parser* CteChar;
	Parser* CteNatural;
	Parser* CteInt;
	Parser* Identifier;
		
	//Syntax parsers
	//Sets section
	Parser* SetEnumeration;
	Parser* SetRange;
	Parser* SetValue;
	Parser* SetExpression;
	Parser* SetRule;
	//Comments, Scanner
	Parser* LexParser;
	Parser* LexCombinator;
	Parser* LexSequence;
	Parser* LexChoice;
	Parser* LexProduction;
	Parser* LexRule;
	//Parser
	Parser* YaccParser;
	Parser* YaccCombinator;
	Parser* YaccAction;
	Parser* YaccSequence;
	Parser* YaccChoice;
	Parser* YaccProduction;
	Parser* YaccRule;

	Parser* Grammar;
	
	//Simplifications
	Parser* T (Parser* _p)		{return _SQ(2, Clear(Ignore(to_ignore)), Token(_p));}
	Parser* T (const string& _w){return _SQ(2, Clear(Ignore(to_ignore)), Word(_w));}
	Parser* I (Parser* _p)		{return _SQ(2, Clear(Ignore(to_ignore)), Ignore(_p));}
	Parser* I (const string& _w){return I(Word(_w));}

public:
	EBNFParser()
	{
		//Sets
		Alpha			= Range('a', 'z') + Range('A', 'Z');
		DecimalDigit	= Range('0', '9');
		AlphaDigit		= Alpha + DecimalDigit + Set('_');
		Whitespaces		= Enumeration(4, ' ', '\t', '\r', '\n');

		//Ignorable parsers
		Separator = Char(Whitespaces);
		Comment   = _SQ(3, Char('#'), _ST(_SQ(2, NotAt(Char('\n')), Any())), Char('\n'));
		to_ignore = _ST(_OR(2, _R(Separator), _R(Comment)));

		//Lexical parsers
		CteString  = _SQ(3, Char('\"'), _ST(_SQ(2, NotAt(Char('\"')), Any())), Char('\"'));
		CteChar    = _SQ(3, Char('\''), Any(), Char('\''));
		CteNatural = _PL(Char(DecimalDigit));
		CteInt	   = _SQ(2, _OP(Char('-')), _PL(Char(DecimalDigit)));
		Identifier = _SQ(2, Char(Alpha), _ST(Char(AlphaDigit)));
		
		//Syntax parsers
		//Sets section
		SetEnumeration = Name("<EN>", true,  Flat(2, _SQ(4, I("["), _OR(2, T(_R(CteChar)), T(_R(Identifier))), _ST(_SQ(2, I(","), _OR(2, T(_R(CteChar)), T(_R(Identifier))))), I("]"))));
		SetRange       = Name("<RG>", false, _SQ(5, I("["), T(_R(CteChar)), I(".."), T(_R(CteChar)), I("]")));
		SetValue       = _OR(5, 
			T(_R(Identifier)),
			_R(SetEnumeration), 
			_R(SetRange),
			Root(1, _SQ(2, T("!"), _R(SetValue))),
			_SQ(3, I("("), _R(SetExpression), I(")"))
		);
		SetExpression  = Root(2, Flat(2, 
			_SQ(2,
				_R(SetValue),
				_OP(
					_SQ(2,
						_OR(3, T("*"), T("+"), T("-")),
						_R(SetExpression)
					)
				)
			)
		));
		SetRule         = Root(1, _SQ(4, T(_R(Identifier)), I("="), _R(SetExpression), I(";")));

		//Comments, Scanner
		LexParser     = _OR(5,
			T(_R(CteString)),
			T(_R(CteChar)),
			T(_R(Identifier)),
			Root(1, _SQ(2, _OR(2, T("^"), T("!")), _R(LexParser))),
			_SQ(3, I("("), _R(LexProduction), I(")"))
		);
		LexCombinator = Root(-1, _SQ(2, _R(LexParser),
			_OP(_OR(4, 
				T("*"), 
				T("+"), 
				T("?"),
				T(_SQ(4, T("{"), T(_R(CteNatural)), _OP(_SQ(2, T(","), _OR(2, T("N"), T(_R(CteNatural))))), T("}")))
			)))
		);
		LexSequence   = Name("&", false, _PL(_R(LexCombinator)));
		LexChoice     = Name("|", false, Flat(2, _SQ(2, _R(LexSequence), _ST(_SQ(2, I("|"), _R(LexSequence))))));
		LexProduction = LexChoice;
		LexRule       = Root(1, _SQ(4, T(_R(Identifier)), I("="), _R(LexProduction), I(";")));

		//Parser
		YaccParser     = _OR(7,
			T(_R(CteString)),
			T(_R(CteChar)),
			T(_R(Identifier)),
			Root(1, _SQ(2, _OR(2, T("^"), T("!")), _R(YaccParser))),
			Name("[]", true, _SQ(3, I("["), _R(YaccProduction), I("]"))),
			Name("<>", true, _SQ(3, I("<"), _R(YaccProduction), I(">"))),
			_SQ(3, I("("), _R(YaccProduction), I(")"))
		);
		YaccCombinator = Root(-1, _SQ(2, _R(YaccParser),
			_OP(_OR(4, 
				T("*"), 
				T("+"), 
				T("?"),
				T(_SQ(4, T("{"), T(_R(CteNatural)), _OP(_SQ(2, T(","), _OR(2, T("N"), T(_R(CteNatural))))), T("}")))
			)))
		);
		YaccAction     = Root(-1, _SQ(2, _R(YaccCombinator),
			_OP(_SQ(2, I("->"), _OR(6, 
				T(_SQ(2, T("&"), T(_R(CteString)))),
				T(_SQ(2, T("?"), T(_R(CteString)))),
				T(_SQ(2, T("_"), T(_R(CteInt)))),
				T(_SQ(2, T("^"), T(_R(CteInt)))),
				T("<<"),
				T(">>")
			)))
			));
		YaccSequence   = Name("&", false, _PL(_R(YaccAction)));
		YaccChoice     = Name("|", false, Flat(2, _SQ(2, _R(YaccSequence), _ST(_SQ(2, I("|"), _R(YaccSequence))))));
		YaccProduction = YaccChoice;
		YaccRule       = Root(1, _SQ(4, T(_R(Identifier)), I("="), _R(YaccProduction), I(";")));

		Grammar = Root(1, _SQ(7, 
			I("GRAMMAR"), 
			T(_R(Identifier)), 
			_OP(Root(1, Flat(2, _SQ(2, T("SETS"),     _PL(_R(SetRule)))))), 
			_OP(Root(1, Flat(2, _SQ(2, T("COMMENTS"), _PL(_R(LexRule)))))), 
			_OP(Root(1, Flat(2, _SQ(2, T("SCANNER"),  _PL(_R(LexRule)))))),
			Root(1, Flat(2, _SQ(2,     T("PARSER"),   _PL(_R(YaccRule))))), 
			T(EndOfInput()))
		);
	}

	virtual ~EBNFParser()
	{
		//Ignorable parsers
		delete Separator;
		delete Comment;
		delete to_ignore;

		//Lexical parsers
		delete CteString;
		delete CteChar;
		delete CteNatural;
		delete CteInt;
		delete Identifier;

		//Syntax parsers
		//Sets section
		delete SetEnumeration;
		delete SetRange;
		delete SetValue;
		delete SetExpression;
		delete SetRule;
		//Comments, Scanner
		delete LexParser;
		delete LexCombinator;
		delete LexSequence;
		delete LexChoice;
		delete LexProduction;
		delete LexRule;
		//Parser
		delete YaccParser;
		delete YaccCombinator;
		delete YaccAction;
		delete YaccSequence;
		delete YaccChoice;
		delete YaccProduction;
		delete YaccRule;
		delete Grammar;
	}

	virtual void Reset()
	{
		//Ignorable parsers
		Separator->Reset();
		Comment->Reset();
		to_ignore->Reset();

		//Lexical parsers
		CteString->Reset();
		CteChar->Reset();
		CteNatural->Reset();
		CteInt->Reset();
		Identifier->Reset();

		//Syntax parsers
		//Sets section
		SetEnumeration->Reset();
		SetRange->Reset();
		SetValue->Reset();
		SetExpression->Reset();
		SetRule->Reset();
		//Comments, Scanner
		LexParser->Reset();
		LexCombinator->Reset();
		LexSequence->Reset();
		LexChoice->Reset();
		LexProduction->Reset();
		LexRule->Reset();
		//Parser
		YaccParser->Reset();
		YaccCombinator->Reset();
		YaccAction->Reset();
		YaccSequence->Reset();
		YaccChoice->Reset();
		YaccProduction->Reset();
		YaccRule->Reset();
		Grammar->Reset();
	}

	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		_tree = 0;
		Result r = Grammar->Parse(_s, _tree);
		if(r)
			r.fail = Error();
		return r;
	}
};

Parser* EBNF_Parser(){return new EBNFParser();}





class EBNFSemantics : public Semantics
{
	bool IsReservedWord(const string& _name)
	{
		//Reserved words
		if((_name == "GRAMMAR") || (_name == "SETS") || (_name == "COMMENTS") || (_name == "SCANNER") || (_name == "PARSER"))
			return true;

		//Special characters
		if((_name == "NL") || (_name == "CR") || (_name == "TB"))
			return true;

		//Special Parsers
		if((_name == "ANY") || (_name == "EMPTY") || (_name == "EOI"))
			return true;

		return false;
	}

	Result ReservedWords(STNode* _group)
	{
		if(!_group)
			return Success();

		for(unsigned int i = 0; i < _group->Sons(); i++)
		{
			if(IsReservedWord(_group->Son(i)->data))
			{
				return Failure(Error(_group->Son(i)->data + " is a reserved word", _group->Son(i)->where));
			}
		}
		return Success();
	}

	Result CheckNames(STNode* _group)
	{
		if(!_group)
			return Success();

		vector<string> names;
		for(unsigned int i = 0; i < _group->Sons(); i++)
		{
			if(find(names.begin(), names.end(), _group->Son(i)->data) != names.end())
			{
				return Failure(Error(_group->Son(i)->data + " already defined", _group->Son(i)->where));
			}
			names.push_back(_group->Son(i)->data);
		}
		return Success();
	}

	Result IntersectNames(STNode* _group1, STNode* _group2)
	{
		if(!_group1 || !_group2)
			return Success();

		for(unsigned int i = 0; i < _group1->Sons(); i++)
		{
			for(unsigned int j = 0; j < _group2->Sons(); j++)
			{
				if(_group1->Son(i)->data == _group2->Son(j)->data)
					return Failure(Error(_group2->Son(j)->data + " already defined", _group2->Son(j)->where));
			}
		}
		return Success();
	}

	bool IsSpecialNodeName(const string& _name)
	{
		//SETS
		if((_name == "+") || (_name == "*") || (_name == "-"))
			return true;

		if((_name == "<EN>") || (_name == "<RG>"))
			return true;

		//PARSERS
		if((_name == "&") || (_name == "|"))
			return true;

		if((_name == "?") || (_name == "+") || (_name == "*") || (_name[0] == '{'))
			return true;

		if((_name == "!") || (_name == "^"))
			return true;

		if((_name == "<>") || (_name == "[]"))
			return true;

		if((_name[0] == '^') || (_name[0] == '_') || (_name[0] == '&') || (_name[0] == '?') || (_name == "<<") || (_name == ">>"))
			return true;

		if((_name[0] == '\"') || (_name[0] == '\''))
			return true;

		if((_name == "NL") || (_name == "CR") || (_name == "TB"))
			return true;

		if((_name == "ANY") || (_name == "EMPTY") || (_name == "EOI"))
			return true;

		return false;
	}

	Result ExistsNamesRec(STNode* _ruleOrSetBody, const vector<string>& _acceptableNames)
	{
		if(!_ruleOrSetBody)
			return Success();

		if(IsSpecialNodeName(_ruleOrSetBody->data))
		{
			for(unsigned int i = 0; i < _ruleOrSetBody->Sons(); i++)
			{
				Result r = ExistsNamesRec(_ruleOrSetBody->Son(i), _acceptableNames);
				if(!r) return r;
			}

			return Success();
		}

		if(find(_acceptableNames.begin(), _acceptableNames.end(), _ruleOrSetBody->data) == _acceptableNames.end())
		{
			string message = IsReservedWord(_ruleOrSetBody->data) ? (_ruleOrSetBody->data + " is a reserved word") : (_ruleOrSetBody->data + " has not been defined");

			return Failure(Error(message, _ruleOrSetBody->where));
		}
		return Success();
	}

	Result ExistsNames(STNode* _group, const vector<string>& _acceptableNames)
	{
		if(!_group)
			return Success();

		for(unsigned int i = 0; i < _group->Sons(); i++)
		{
			//Get the rule or set
			STNode* ruleOrSet = _group->Son(i);

			//data = rule or set Name | Son(0) -> Body
			Result r = ExistsNamesRec(ruleOrSet->Son(0), _acceptableNames);
			if(!r) return r;
		}
		return Success();
	}
	
	vector<string> CollectNames(STNode* _group)
	{
		vector<string> names;

		if(!_group)
			return names;

		for(unsigned int i = 0; i < _group->Sons(); i++)
		{
			names.push_back(_group->Son(i)->data);
		}
		return names;
	}

	vector<string> Merge(const vector<string>& _set1, const vector<string>& _set2)
	{
		vector<string> r = _set1;
		r.insert(r.end(), _set2.begin(), _set2.end());
		return r;
	}
	

	Result RepeatParsersRec(STNode* _ruleBody)
	{
		if(!_ruleBody)
			return Success();

		if(_ruleBody->data[0] == '{')
		{
			size_t comma_pos = _ruleBody->data.find(',');
			if(comma_pos != string::npos)
			{
				int minN = atoi(_ruleBody->data.substr(1, comma_pos - 1).c_str());
				
				string maxN_str = _ruleBody->data.substr(comma_pos, _ruleBody->data.size() - 1 - comma_pos);
				if(maxN_str != "N")
				{
					int maxN = atoi(maxN_str.c_str());

					if(minN > maxN)
						return Failure(Error("Repeat parser bad formed (min > max)", _ruleBody->where));
				}
			}
		}

		for(unsigned int i = 0; i < _ruleBody->Sons(); i++)
		{
			Result r = RepeatParsersRec(_ruleBody->Son(i));
			if(!r) return r;
		}
		return Success();
	}

	Result RepeatParsers(STNode* _group)
	{
		if(!_group)
			return Success();

		for(unsigned int i = 0; i < _group->Sons(); i++)
		{
			//Get the rule or set
			STNode* rule = _group->Son(i);

			//data = rule or set Name | Son(0) -> Body
			Result r = RepeatParsersRec(rule->Son(0));
			if(!r) return r;
		}
		return Success();
	}

	Result StartRule(STNode* _parsers)
	{
		for(unsigned int i = 0; i < _parsers->Sons(); i++)
		{
			if(_parsers->Son(i)->data == "start")
				return Success();
		}
		return Failure(Error("\"start\" rule has not been defined"));
	}

	STNode* GetRuleNode(STNode* _group, const string& _name)
	{
		if(!_group)
			return 0;

		for(unsigned int i = 0; i < _group->Sons(); i++)
		{
			STNode* rule = _group->Son(i);

			if(rule->data == _name)
				return rule;
		}
		return 0;
	}

	Result LeftRecursionRec(const vector<string>& _ruleNames, STNode* _group, STNode* _rule)
	{
		if(!_rule)
			return Success();
		
		if(_rule->IsLeaf())
		{
			if(find(_ruleNames.begin(), _ruleNames.end(), _rule->data) != _ruleNames.end())
				return Failure(Error(_rule->data + " has a left recursive derivation", _rule->where));

			if(IsSpecialNodeName(_rule->data))
				return Success();

			STNode* newRule = GetRuleNode(_group, _rule->data);
			if(!newRule)
				return Success();

			vector<string> newNames = _ruleNames;
			newNames.push_back(newRule->data);
			return LeftRecursionRec(newNames, _group, newRule);
		}

		if(_rule->data == "|")
		{
			Error e;
			for(unsigned int i = 0; i < _rule->Sons(); i++)
			{
				Result r = LeftRecursionRec(_ruleNames, _group, _rule->Son(i));
				if(!r)
					return r;
				e += r.fail;
			}
			return Success(e);
		}

		return LeftRecursionRec(_ruleNames, _group, _rule->Son(0));
	}

	Result LeftRecursion(STNode* _group)
	{
		if(!_group)
			return Success();

		for(unsigned int i = 0; i < _group->Sons(); i++)
		{
			//Get the rule or set
			STNode* rule = _group->Son(i);

			vector<string> ruleNames;
			ruleNames.push_back(rule->data);

			Result r = LeftRecursionRec(ruleNames, _group, rule);
			if(!r)
				return r;
		}
		return Success();
	}

#define VERIFY(X) r = X ; if(!r) return r
public:
	virtual Result Check(STNode* _st)
	{
		//Obtain sub-trees with data about sets, comments, scanners and parsers
		unsigned int index = 0;
		STNode* sets = 0;
		STNode* comments = 0;
		STNode* scanners = 0;
		STNode* parsers = 0;

		if(_st->childs[index]->data == "SETS")
			sets = _st->childs[index++];
		if(_st->childs[index]->data == "COMMENTS")
			comments = _st->childs[index++];
		if(_st->childs[index]->data == "SCANNER")
			scanners = _st->childs[index++];
		parsers = _st->childs[index++];

		//1.- Verify no rule is named equal to a reserved word
		Result r;
		VERIFY(ReservedWords(sets));
		VERIFY(ReservedWords(comments));
		VERIFY(ReservedWords(scanners));
		VERIFY(ReservedWords(parsers));

		//2.- Verify names are not repeated
		VERIFY(CheckNames(sets));
		VERIFY(CheckNames(comments));
		VERIFY(CheckNames(scanners));
		VERIFY(CheckNames(parsers));

		VERIFY(IntersectNames(sets, comments));
		VERIFY(IntersectNames(sets, scanners));
		VERIFY(IntersectNames(sets, parsers));
		VERIFY(IntersectNames(comments, scanners));
		VERIFY(IntersectNames(comments, parsers));
		VERIFY(IntersectNames(scanners, parsers));

		//3.- Every referenced rule does exists and rules only reference lower sets (parser references parser and lexical, etc)
		VERIFY(ExistsNames(sets, CollectNames(sets)));
		VERIFY(ExistsNames(comments, CollectNames(sets)));
		VERIFY(ExistsNames(scanners, CollectNames(sets)));
		VERIFY(ExistsNames(parsers, Merge(Merge(CollectNames(sets), CollectNames(scanners)), CollectNames(parsers))));

		//4.- Verify Repeat parsers {x,y} -> x <= y
		VERIFY(RepeatParsers(comments));
		VERIFY(RepeatParsers(scanners));
		VERIFY(RepeatParsers(parsers));

		//5.- "start" rule does exists
		VERIFY(StartRule(parsers));

		//6.- No left-recursion allowed
		VERIFY(LeftRecursion(comments));
		VERIFY(LeftRecursion(scanners));
		VERIFY(LeftRecursion(parsers));

		return Success();
	}
};
Semantics* EBNF_Semantics(){return new EBNFSemantics();}



class EBNFCodeGenerator : public CodeGenerator
{
	void Tabs(FILE* _file, unsigned int _tabs)
	{
		for(unsigned int i = 0; i < _tabs; i++)
			fprintf(_file, "    ");
	}

	void TranslateChar(FILE* _file, STNode* _char)
	{
		if(_char->data == "NL")
			fprintf(_file, "'\\n'");
		else if(_char->data == "CR")
			fprintf(_file, "'\\r'");
		else if(_char->data == "TB")
			fprintf(_file, "'\\t'");
		else
			fprintf(_file, "%s", _char->data.c_str());
	}

	void GenerateSetExpression(FILE* _file, STNode* _expr, unsigned int _level)
	{
		if((_expr->data == "+") || (_expr->data == "*") || (_expr->data == "-"))
		{
			GenerateSetExpression(_file, _expr->Son(0), _level + 1);
			fprintf(_file, " %s ", _expr->data.c_str());
			GenerateSetExpression(_file, _expr->Son(1), _level + 1);
		}
		else if(_expr->data == "<EN>")
		{
			fprintf(_file, "Enumeration(%d, ", _expr->Sons());
			for(unsigned int i = 0; i < _expr->Sons(); i++)
			{
				TranslateChar(_file, _expr->Son(i));

				if(i != (_expr->Sons() - 1))
					fprintf(_file, ", ");
			}
			fprintf(_file, ")");
		}
		else if(_expr->data == "<RG>")
		{
			fprintf(_file, "Range(");
			TranslateChar(_file, _expr->Son(0));
			fprintf(_file, ", ");
			TranslateChar(_file, _expr->Son(1));
			fprintf(_file, ")");
		}
		else
		{
			fprintf(_file, "%s", _expr->data.c_str());
		}
	}

	void GenerateSet(FILE* _file, STNode* _set)
	{
		Tabs(_file, 2); fprintf(_file, "%s = ", _set->data.c_str());
		GenerateSetExpression(_file, _set->Son(0), 0);
		fprintf(_file, ";\n\n");
	}

	void GenerateSets(FILE* _file, STNode* _sets)
	{
		if(!_sets)
			return;

		for(unsigned int i = 0; i < _sets->Sons(); i++)
			GenerateSet(_file, _sets->Son(i));
	}

	bool Contains(STNode* _group, const string& _name)
	{
		if(!_group)
			return false;

		for(unsigned int i = 0; i < _group->Sons(); i++)
		{
			if(_group->Son(i)->data == _name)
				return true;
		}
		return false;
	}

	void GenerateRuleParser(FILE* _file, STNode* _rule, unsigned int _level, STNode* _sets, STNode* _scanner)
	{
		if(_rule->data == "&")
		{
			Tabs(_file, 3 + _level); fprintf(_file, "Sequence(%d,\n", _rule->Sons());
			for(unsigned int i = 0; i < _rule->Sons(); i++)
			{
				GenerateRuleParser(_file, _rule->Son(i), _level + 1, _sets, _scanner);
				if(i != (_rule->Sons() - 1))
				{
					Tabs(_file, 3 + _level + 1); fprintf(_file, ",\n");
				}
			}
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data == "|")
		{
			Tabs(_file, 3 + _level); fprintf(_file, "Choice(%d,\n", _rule->Sons());
			for(unsigned int i = 0; i < _rule->Sons(); i++)
			{
				GenerateRuleParser(_file, _rule->Son(i), _level + 1, _sets, _scanner);
				if(i != (_rule->Sons() - 1))
				{
					Tabs(_file, 3 + _level + 1); fprintf(_file, ",\n");
				}
			}
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data == "?")
		{
			Tabs(_file, 3 + _level); fprintf(_file, "Optional(\n");
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data == "+")
		{
			Tabs(_file, 3 + _level); fprintf(_file, "Plus(\n");
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data == "*")
		{
			Tabs(_file, 3 + _level); fprintf(_file, "Star(\n");
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data[0] == '{')
		{
			int minN = 0;
			int maxN = 0;

			size_t comma_pos = _rule->data.find(',');
			if(comma_pos != string::npos)
			{
				minN = atoi(_rule->data.substr(1, comma_pos - 1).c_str());
				string maxN_str = _rule->data.substr(comma_pos, _rule->data.size() - 1 - comma_pos);
				maxN = (maxN_str == "N") ? -1 : atoi(maxN_str.c_str());
			}
			else
			{
				minN = maxN = atoi(_rule->data.substr(1, _rule->data.size() - 2).c_str());
			}

			Tabs(_file, 3 + _level); fprintf(_file, "Repeat(%d, %d,\n", minN, maxN);
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data == "!")
		{
			Tabs(_file, 3 + _level); fprintf(_file, "NotAt(\n");
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data == "^")
		{
			Tabs(_file, 3 + _level); fprintf(_file, "At(\n");
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data == "<>")
		{
			Tabs(_file, 3 + _level); fprintf(_file, "Ignore(\n");
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data == "[]")
		{
			Tabs(_file, 3 + _level); fprintf(_file, "Token(\n");
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data[0] == '^')
		{
			Tabs(_file, 3 + _level);
			int	index = atoi(_rule->data.substr(1).c_str());
			fprintf(_file, "Root(%d,\n", index);
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data[0] == '_')
		{
			Tabs(_file, 3 + _level);
			int	index = atoi(_rule->data.substr(1).c_str());
			fprintf(_file, "Flat(%d,\n", index);
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data[0] == '&')
		{
			Tabs(_file, 3 + _level);
			string name = _rule->data.substr(1);
			fprintf(_file, "Name(%s, true,\n", name.c_str());
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data[0] == '?')
		{
			Tabs(_file, 3 + _level);
			string name = _rule->data.substr(1);
			fprintf(_file, "Name(%s, false,\n", name.c_str());
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data == "<<")
		{
			Tabs(_file, 3 + _level);
			fprintf(_file, "Left(\n");
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data == ">>")
		{
			Tabs(_file, 3 + _level);
			fprintf(_file, "Right(\n");
			GenerateRuleParser(_file, _rule->Son(0), _level + 1, _sets, _scanner);
			Tabs(_file, 3 + _level); fprintf(_file, ")\n");
		}
		else if(_rule->data[0] == '\"')
		{
			Tabs(_file, 3 + _level);
			if(!_scanner)
				fprintf(_file, "Word(%s)\n", _rule->data.c_str());
			else
				fprintf(_file, "S(Word(%s))\n", _rule->data.c_str());
		}
		else if(_rule->data[0] == '\'' || _rule->data == "NL" || _rule->data == "TB")
		{
			Tabs(_file, 3 + _level);
			if(!_scanner)
			{
				fprintf(_file, "Char("); TranslateChar(_file, _rule); fprintf(_file, ")\n");
			}
			else
			{
				fprintf(_file, "S(Char("); TranslateChar(_file, _rule); fprintf(_file, "))\n");
			}
		}
		else if(_rule->data == "ANY")
		{
			Tabs(_file, 3 + _level);
			if(!_scanner)
				fprintf(_file, "Any()\n");
			else
				fprintf(_file, "S(Any())\n");
		}
		else if(_rule->data == "EMPTY")
		{
			Tabs(_file, 3 + _level);
			if(!_scanner)
				fprintf(_file, "Empty()\n");
			else
				fprintf(_file, "S(Empty())\n");
		}
		else if(_rule->data == "EOI")
		{
			Tabs(_file, 3 + _level);
			if(!_scanner)
				fprintf(_file, "EndOfInput()\n");
			else
				fprintf(_file, "S(EndOfInput())\n");
		}
		else
		{
			Tabs(_file, 3 + _level);
			if(Contains(_sets, _rule->data.c_str()))
			{
				fprintf(_file, "Char(%s)\n", _rule->data.c_str());
			}
			else if(Contains(_scanner, _rule->data))
			{
				fprintf(_file, "S(Token(Reference(&%s)))\n", _rule->data.c_str());
			}
			else
			{
				fprintf(_file, "Reference(&%s)\n", _rule->data.c_str());
			}
		}
	}

	void GenerateRule(FILE* _file, STNode* _rule, STNode* _sets, STNode* _scanner)
	{
		Tabs(_file, 2); fprintf(_file, "%s = \n", _rule->data.c_str());
		GenerateRuleParser(_file, _rule->Son(0), 0, _sets, _scanner);
		Tabs(_file, 2); fprintf(_file, ";\n\n");
	}

	void GenerateRules(FILE* _file, STNode* _rules, STNode* _sets, STNode* _scanner)
	{
		if(!_rules)
			return;

		for(unsigned int i = 0; i < _rules->Sons(); i++)
			GenerateRule(_file, _rules->Son(i), _sets, _scanner);
	}

	void GenerateToIgnore(FILE* _file, STNode* _comments)
	{
		Tabs(_file, 2);
		if(!_comments)
		{
			fprintf(_file, "_to_ignore = Empty();");
		}
		else
		{
			Tabs(_file, 2);
			fprintf(_file, "_to_ignore = Star(Choice(%d, ", _comments->Sons());
			for(unsigned int i = 0; i <_comments->Sons(); i++)
			{
				fprintf(_file, "Reference(&%s)", _comments->Son(i)->data.c_str());
				if(i != _comments->Sons() - 1)
					fprintf(_file, ", ");
			}
			fprintf(_file, "));\n");
		}
	}

	void GenerateStatement(FILE* _file, STNode* _group, unsigned int _level, const string& _statement)
	{
		if(!_group)
			return;

		for(unsigned int i = 0; i < _group->Sons(); i++)
		{
			Tabs(_file, _level);
			fprintf(_file, _statement.c_str(), _group->Son(i)->data.c_str());
		}
	}

	void GenerateParser(FILE* _file, STNode* _ast)
	{
		fprintf(_file, "class %sParser : public Parser\n", _ast->data.c_str());
		fprintf(_file, "{\n");

		unsigned int index = 0;
		STNode* sets = 0;
		STNode* comments = 0;
		STNode* scanners = 0;
		STNode* parsers = 0;

		if(_ast->childs[index]->data == "SETS")
			sets = _ast->childs[index++];
		if(_ast->childs[index]->data == "COMMENTS")
			comments = _ast->childs[index++];
		if(_ast->childs[index]->data == "SCANNER")
			scanners = _ast->childs[index++];
		parsers = _ast->childs[index++];

		if(sets)
		{
			Tabs(_file, 1); fprintf(_file, "//Sets\n");
			GenerateStatement(_file, sets, 1, "Set %s;\n");
			fprintf(_file, "\n");
		}
		if(comments)
		{
			Tabs(_file, 1); fprintf(_file, "//Ignorable parsers\n");
			GenerateStatement(_file, comments, 1, "Parser* %s;\n");
			fprintf(_file, "\n");
		}
		if(scanners)
		{
			Tabs(_file, 1); fprintf(_file, "//Scanner parsers\n");
			GenerateStatement(_file, scanners, 1, "Parser* %s;\n");
			fprintf(_file, "\n");
		}
		Tabs(_file, 1); fprintf(_file, "//Syntax parsers\n");
		GenerateStatement(_file, parsers,  1, "Parser* %s;\n");
		fprintf(_file, "\n");

		Tabs(_file, 1); fprintf(_file, "//Helpers\n");
		Tabs(_file, 1); fprintf(_file, "Parser* _to_ignore;\n");
		Tabs(_file, 1); fprintf(_file, "Parser* S(Parser* _p){return Sequence(2, Clear(Ignore(_to_ignore)), _p);}\n");
		
		fprintf(_file, "\n");
		fprintf(_file, "public:\n");

		//Constructor
		Tabs(_file, 1); fprintf(_file, "%sParser()\n", _ast->data.c_str());
		Tabs(_file, 1); fprintf(_file, "{\n");
		
		if(sets)
		{
			Tabs(_file, 2); fprintf(_file, "//Sets\n");
			GenerateSets(_file, sets);
			fprintf(_file, "\n");
		}

		Tabs(_file, 2); fprintf(_file, "//Ignorable parsers\n");
		if(comments) GenerateRules(_file, comments, sets, 0);
		GenerateToIgnore(_file, comments);
		fprintf(_file, "\n\n");

		if(scanners)
		{
			Tabs(_file, 2); fprintf(_file, "//Scanner parsers\n");
			GenerateRules(_file, scanners, sets, 0);
			fprintf(_file, "\n");
		}

		Tabs(_file, 2); fprintf(_file, "//Syntax parsers\n");
		GenerateRules(_file, parsers, sets, scanners);

		Tabs(_file, 1); fprintf(_file, "}\n");
		fprintf(_file, "\n");

		//Destructor
		Tabs(_file, 1); fprintf(_file, "virtual ~%sParser()\n", _ast->data.c_str());
		Tabs(_file, 1); fprintf(_file, "{\n");
		
		Tabs(_file, 2); fprintf(_file, "//Ignorable parsers\n");
		if(comments)
		{
			GenerateStatement(_file, comments, 2, "delete %s;\n");			
		}
		Tabs(_file, 2); fprintf(_file, "delete _to_ignore;\n");
		fprintf(_file, "\n");
		if(scanners)
		{
			Tabs(_file, 2); fprintf(_file, "//Scanner parsers\n");
			GenerateStatement(_file, scanners, 2, "delete %s;\n");
			fprintf(_file, "\n");
		}
		Tabs(_file, 2); fprintf(_file, "//Syntax parsers\n");
		GenerateStatement(_file, parsers,  2,  "delete %s;\n");
		Tabs(_file, 1); fprintf(_file, "}\n");
		fprintf(_file, "\n");

		//Reset
		Tabs(_file, 1); fprintf(_file, "virtual void Reset()\n");
		Tabs(_file, 1); fprintf(_file, "{\n");
		Tabs(_file, 2); fprintf(_file, "//Ignorable parsers\n");
		if(comments)
		{
			GenerateStatement(_file, comments, 2, "%s->Reset();\n");			
		}
		Tabs(_file, 2); fprintf(_file, "delete _to_ignore;\n");
		fprintf(_file, "\n");
		if(scanners)
		{
			Tabs(_file, 2); fprintf(_file, "//Scanner parsers\n");
			GenerateStatement(_file, scanners, 2, "%s->Reset();\n");
			fprintf(_file, "\n");
		}
		Tabs(_file, 2); fprintf(_file, "//Syntax parsers\n");
		GenerateStatement(_file, parsers,  2,  "%s->Reset();\n");
		Tabs(_file, 1); fprintf(_file, "}\n");
		fprintf(_file, "\n");

		//Parse
		Tabs(_file, 1); fprintf(_file, "virtual Result Parse(Stream* _s, STNode*& _tree)\n");
		Tabs(_file, 1); fprintf(_file, "{\n");
		Tabs(_file, 2); fprintf(_file, "Result r = start->Parse(_s, _tree);\n");
		Tabs(_file, 2); fprintf(_file, "if(r)\n");
		Tabs(_file, 3); fprintf(_file, "r.Clear();\n");
		Tabs(_file, 2); fprintf(_file, "return r;\n");
		Tabs(_file, 1); fprintf(_file, "}\n");
		fprintf(_file, "};\n\n");

		fprintf(_file, "Parser* %s_Parser()\n", _ast->data.c_str());
		fprintf(_file, "{\n");
		Tabs(_file, 1); fprintf(_file, "return new %sParser();\n", _ast->data.c_str());
		fprintf(_file, "}\n");
	}


	void GenerateHeader(FILE* _file, const string& _name)
	{
		fprintf(_file, "#ifndef __%s_H__\n", _name.c_str());
		fprintf(_file, "#define __%s_H__\n", _name.c_str());
		fprintf(_file, "\n");
		fprintf(_file, "#include \"Languages.h\"\n");
		fprintf(_file, "\n");
		fprintf(_file, "Parser* %s_Parser();\n", _name.c_str());
		fprintf(_file, "\n");
		fprintf(_file, "#endif\n");
	}

	void GenerateBody(FILE* _file, STNode* _ast)
	{
		fprintf(_file, "#include \"%s.h\"\n", _ast->data.c_str());
		fprintf(_file, "\n");
		fprintf(_file, "//PARSER\n");
		GenerateParser(_file, _ast);
		fprintf(_file, "\n");
	}

public:
	virtual Result Generate(STNode* _st, const string& _path)
	{
		FILE* file = 0;
		string fileName = "";

		//Header
		fileName = _path + _st->data + ".h";
		fopen_s(&file, fileName.c_str(), "w");
		if(!file)
			return Failure(Error(fileName + " could not be open"));
		GenerateHeader(file, _st->data);
		fclose(file);

		//Body
		fileName = _path + _st->data + ".cpp";
		fopen_s(&file, fileName.c_str(), "w");
		if(!file)
			return Failure(Error(fileName + " could not be open"));
		GenerateBody(file, _st);
		fclose(file);

		return Success();
	}
};

CodeGenerator* EBNF_CodeGenerator(){return new EBNFCodeGenerator();}
