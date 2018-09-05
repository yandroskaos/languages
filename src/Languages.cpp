#include "Languages.h"
#include <cstdarg>
#include <map>
#include <stack>
#include <algorithm>


Position::Position(unsigned int _row, unsigned int _column)
	: row(_row), column(_column)
{
}

bool Position::operator<(const Position& _p) const
{
	return (row < _p.row) || ((row == _p.row) && (column < _p.column)); 
}

bool Position::operator>(const Position& _p) const
{
	return (row > _p.row) || ((row == _p.row) && (column > _p.column)); 
}

bool Position::operator==(const Position& _p) const
{
	return row == _p.row && column == _p.column;
}

bool Position::operator!=(const Position& _p) const
{
	return !operator==(_p);
}


/**
* @brief Stream Implementation.
*/
class StreamImpl : public Stream
{
	const char*  input;
	size_t       size;
	bool         deleteInput;

	unsigned int head;
	Position     where;

	mutable map<Position, unsigned int> cache;
	void AddToCache() const
	{
		if(cache.find(where) == cache.end())
			cache.insert(pair<Position, unsigned int>(where, head));
	}
	bool RetrieveFromCache(const Position& _where)
	{
		map<Position, unsigned int>::iterator i = cache.find(_where);
		if(i != cache.end())
		{
			head  = i->second;
			where = _where;
			return true;
		}
		return false;
	}
public:
	StreamImpl(const char* _input, size_t _size, bool _deleteInput)
		: input(_input), size(_size), deleteInput(_deleteInput), head(0), where(Position(1, 1))
	{
	}

	StreamImpl::~StreamImpl()
	{
		if(deleteInput)
			free((void*)input);
	}

	virtual char Get() const	
	{
		if(AtEnd())
			return 0;

		return *(input + head);
	}

	virtual void Next()
	{
		if(AtEnd())
			return;

		if(Get() == '\n')
		{
			where.row++;
			where.column = 1;
		} 
		else
		{
			where.column++;
		}

		head++;
	}

	virtual bool AtEnd() const
	{
		return head >= size;
	}

	virtual Position Where() const
	{
		AddToCache();
		return where;
	}

	virtual bool Goto(const Position& _newPosition)
	{
		if(RetrieveFromCache(_newPosition))
			return true;

		//Save
		Position savedPosition = where;
		unsigned savedHead = head;

		//Start
		where = Position(1, 1);
		head = 0;

		while(where < _newPosition)
			Next();

		if(where != _newPosition)
		{
			where = savedPosition;
			head = savedHead;
			return false;
		}

		return true;
	}
};

Stream::~Stream() {}

Stream* MemoryStream(const char* _input, size_t _size)
{
	return new StreamImpl(_input, _size, false);
}

Stream* FileStream(const char* _fileName)
{
	FILE* f = 0;
	fopen_s(&f, _fileName, "rb");
	if(!f)
		return 0;

	if(fseek(f, 0, SEEK_END))
	{
		fclose(f);
		return 0;
	}
	size_t size = (size_t)ftell(f);
	if(fseek(f, 0, SEEK_SET))
	{
		fclose(f);
		return 0;
	}

	char* input = (char*)malloc(size + 1);
	if(!input)
	{
		fclose(f);
		return 0;
	}
	memset(input, 0, size + 1);
	
	if(fread(input, 1, size, f) != size)
	{
		free(input);
		fclose(f);
		return 0;
	}

	fclose(f);

	StreamImpl* s = new StreamImpl(input, size, true);
	if(!s)
	{
		free(input);
	}

	return s;
}







STNode::STNode(const Position& _where, const string& _data)
	: where(_where), data(_data){}

STNode::~STNode()
{
	for(unsigned int i = 0; i < childs.size(); i++)
	{
		delete childs[i];
	}
}

bool STNode::HasData()
{
	return data != "";
}

bool STNode::IsLeaf()
{
	return Sons() == 0;
}

void STNode::Merge(STNode* _son, unsigned int _where)
{
	if(_son && _where <= Sons())
	{
		childs.insert(childs.begin() + _where, _son->childs.begin(), _son->childs.end());
		_son->UnlinkAll();
	}
}

void STNode::AddSon(STNode* _son)
{
	if(_son)
		childs.push_back(_son);
}

STNode* STNode::Son(unsigned int _index)
{
	return (_index < Sons()) ? childs[_index] : 0;
}

unsigned int STNode::Sons()
{
	return childs.size();
}

void STNode::Unlink(STNode* _son)
{
	vector<STNode*>::iterator i = find(childs.begin(), childs.end(), _son);
	if(i != childs.end())
		childs.erase(i);
}

void STNode::UnlinkAll()
{
	childs.clear();
}





static void PreWalk_Impl(STNode* _root, unsigned int _level, TreeVisitor* _visitor)
{
	if(!_root)
		return;

	if(!_visitor->Visit(_root, _level))
		return;

	for(unsigned int i = 0; i < _root->Sons(); i++)
		PreWalk_Impl(_root->childs[i], _level + 1, _visitor);
}
void PreWalk(STNode* _root, TreeVisitor* _visitor) {PreWalk_Impl(_root, 0, _visitor);}

static void InWalk_Impl(STNode* _root, unsigned int _level, TreeVisitor* _visitor)
{
	if(!_root)
		return;

	if(!_root->IsLeaf())
		InWalk_Impl(_root->childs[0], _level + 1, _visitor);

	if(!_visitor->Visit(_root, _level))
		return;

	for(unsigned int i = 1; i < _root->Sons(); i++)
		InWalk_Impl(_root->childs[i], _level, _visitor);
}
void InWalk(STNode* _root, TreeVisitor* _visitor) {InWalk_Impl(_root, 0, _visitor);}

static void PostWalk_Impl(STNode* _root, unsigned int _level, TreeVisitor* _visitor)
{
	if(!_root)
		return;

	for(unsigned int i = 0; i < _root->Sons(); i++)
		PostWalk_Impl(_root->childs[i], _level + 1, _visitor);

	_visitor->Visit(_root, _level);
}
void PostWalk(STNode* _root, TreeVisitor* _visitor) {PostWalk_Impl(_root, 0, _visitor);}







bool Set::contains(const vector<char>& _set, const char& _element) const
{
	return find(_set.begin(), _set.end(), _element) != _set.end();
}

Set::Set(){}

Set::Set(char _c)
{
	elements.push_back(_c);
}

Set Set::operator+(const Set& _set) const
{
	//Union starts with one set
	Set union_set(*this);

	for (size_t i = 0; i < _set.elements.size(); i++)
	{
		if(!contains(elements, _set.elements[i]))
		{
			//Element NOT FOUND, so add to union
			union_set.elements.push_back(_set.elements[i]);
		}
	}

	return union_set;
}

Set Set::operator*(const Set& _set) const
{
	//Intersection starts empty
	Set intersection_set;

	for (size_t i = 0; i < _set.elements.size(); i++)
	{
		if(contains(elements, _set.elements[i]))
		{
			//Element FOUND, so add to intersection
			intersection_set.elements.push_back(_set.elements[i]);
		}
	}

	return intersection_set;
}

Set Set::operator-(const Set& _set) const
{
	//Intersection starts empty
	Set difference_set;

	for (size_t i = 0; i < elements.size(); i++)
	{
		if(!contains(_set.elements, elements[i]))
		{
			//Element NOT FOUND, so add to difference
			difference_set.elements.push_back(_set.elements[i]);
		}
	}

	return difference_set;
}

bool Set::operator<(const Set& _set) const
{
	//Test this contained in _set
	for (size_t i = 0; i < elements.size(); i++)
	{
		if(!contains(_set.elements, elements[i]))
			return false;
	}
	return true;
}

bool Set::operator>(const Set& _set) const
{
	//Test _set contained in this
	for (size_t i = 0; i < _set.elements.size(); i++)
	{
		if(!contains(elements, _set.elements[i]))
			return false;
	}
	return true;
}

bool Set::operator==(const Set& _set) const
{
	//Equal is _set contained in *this, and *this contained in _set
	return (_set < *this) && (*this < _set);
}

bool Set::operator!=(const Set& _set) const
{
	return ! operator==(_set);
}

string Set::Name()
{
	string result = "[";
	for(unsigned int i = 0; i < elements.size(); i++)
		result += elements[i];
	result += "]";
	return result;
}

Set Range(char _begin, char _end)
{
	Set result;
	for (char i = _begin; i <= _end; i++)
		result = result + i;
	return result;
}

Set Enumeration(unsigned int _number, ...)
{
	va_list arguments;                     
	Set result;

	va_start(arguments, _number);           
	for(unsigned int i = 0; i < _number; i++)
	{
		result = result + va_arg(arguments, char); 
	}
	va_end(arguments);
		
	return result;
}





Error::Error(const string& _expected, const Position& _where)
	: where(_where)
{
	expected.push_back(_expected);
}

Error& Error::operator+=(const Error& _error)
{
	//Replace
	if(_error.where > where)
	{
		*this = _error;
	}
	
	//Add
	if(_error.where == where)
	{
		for(unsigned int i = 0; i < _error.expected.size(); i++)
		{
			if(find(expected.begin(), expected.end(), _error.expected[i]) == expected.end()) 
				expected.push_back(_error.expected[i]);
		}
	}

	return *this;
}

Error::operator bool()
{
	return where != Position() || !expected.empty();
}



Result::Result(bool _match, const Error& _fail)
	: match(_match), fail(_fail)
{}

Result::operator bool()
{
	return match;
}

Result Result::operator!()
{
	return Result(!match, fail);
}

Result Result::Clear()
{
	fail = Error();
	return *this;
}

Result Success(const Error& _fail)
{
	return Result(true, _fail);
}

Result Failure(const Error& _fail)
{
	return Result(false, _fail);
}






Parser::~Parser() {}

class CharParser : public Parser
{
	Set set;
public:
	CharParser(const Set& _set)
		: set(_set)
	{
	}
	virtual void Reset()
	{
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		_tree = 0;

		if(_s->AtEnd())
			return Failure(Error(set.Name(), _s->Where()));

		char c = _s->Get();
		if(set > c)
		{
			_tree = new STNode(_s->Where(), string(1, c));

			_s->Next();
			return Success();
		}

		return Failure(Error(set.Name(), _s->Where()));
	}
};

class WordParser : public Parser
{
	string word;
public:
	WordParser(const string& _word)
		: word(_word)
	{
	}
	virtual void Reset()
	{
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		_tree = 0;

		Position start = _s->Where();

		if(_s->AtEnd())
			return Failure(Error(word, start));

		for(unsigned int i = 0; i < word.size(); i++)
		{
			if(word[i] != _s->Get())
			{
				_s->Goto(start);
				return Failure(Error(word, _s->Where()));
			}
			else
			{
				_s->Next();
			}
		}

		_tree = new STNode(start, word);
		return Success();
	}
};

class EmptyParser : public Parser
{
public:
	virtual void Reset()
	{
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		_tree = 0;
		return Success();
	}
};

class AnyParser : public Parser
{
public:
	virtual void Reset()
	{
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		_tree = 0;
		if(_s->AtEnd())
			return Failure(Error("ANY", _s->Where()));

		_tree = new STNode(_s->Where(), string(1, _s->Get()));
		_s->Next();
		return Success();
	}
};

class EndOfInputParser : public Parser
{
public:
	virtual void Reset()
	{
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		_tree = 0;
		return _s->AtEnd() ? 
			Success() : 
			Failure(Error("EOI", _s->Where()));
	}
};




class CheckParser : public Parser
{
	Parser* p;
	bool present;
public:
	CheckParser(Parser* _p, bool _present)
		: p(_p), present(_present)
	{
	}
	virtual ~CheckParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		Position start = _s->Where();
	
		Result r = p->Parse(_s, _tree);

		delete _tree;
		_tree = 0;

		_s->Goto(start);
		
		return present ? r : !r;
	}
};


static STNode* Colapse(STNode* _node)
{
	if(_node->IsLeaf())
	{
		delete _node;
		return 0;
	}
	else if(_node->Sons() == 1)
	{
		STNode* son = _node->Son(0);
		_node->UnlinkAll();
		delete _node;
		_node = son;
	}

	return _node;
}

class RepeatParser : public Parser
{
	Parser* p;
	int minN;
	int maxN;

public:
	RepeatParser(Parser* _p, int _minN, int _maxN)
		: p(_p), minN(_minN), maxN(_maxN)
	{
	}
	virtual ~RepeatParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		_tree = 0;
		Position start = _s->Where();

		STNode* repetition = new STNode(_s->Where());
		Error e;

		//Mandatory part
		int i = 0;
		for(; i < minN; i++)
		{
			STNode* t = 0;
			Result r = p->Parse(_s, t);
			e += r.fail;
			if(!r)
			{
				_s->Goto(start);
				delete repetition;
				return Failure(e);
			}

			repetition->AddSon(t);
		}

		//Optional part
		for(; (i < maxN) || (maxN == -1); i++)
		{
			STNode* t = 0;
			Result r = p->Parse(_s, t);
			e += r.fail;
			if(!r)
				break;
			repetition->AddSon(t);
		}

		_tree = Colapse(repetition);
		return Success(e);
	}
};

class SequenceParser : public Parser
{
	vector<Parser*> ps;
public:
	SequenceParser(const vector<Parser*> _ps)
		: ps(_ps)
	{
	}
	virtual ~SequenceParser()
	{
		for(unsigned int i = 0; i < ps.size(); i++)
			delete ps[i];
	}
	virtual void Reset()
	{
		for(unsigned int i = 0; i < ps.size(); i++)
			ps[i]->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		_tree = 0;
		Position start = _s->Where();

		STNode* sequence = new STNode(_s->Where());
		Error e;

		for(unsigned int i = 0; i < ps.size(); i++)
		{
			Parser* p = ps[i];
			STNode* t = 0;
			Result r = p->Parse(_s, t);
			e += r.fail;			
			if(!r)
			{
				_s->Goto(start);
				delete sequence;
				return Failure(e);
			}

			sequence->AddSon(t);
		}

		_tree = Colapse(sequence);
		return Success(e);
	}
};

class ChoiceParser : public Parser
{
	vector<Parser*> ps;
public:
	ChoiceParser(const vector<Parser*> _ps)
		: ps(_ps)
	{
	}
	virtual ~ChoiceParser()
	{
		for(unsigned int i = 0; i < ps.size(); i++)
			delete ps[i];
	}
	virtual void Reset()
	{
		for(unsigned int i = 0; i < ps.size(); i++)
			ps[i]->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		_tree = 0;
		Error e;
		
		for(unsigned int i = 0; i < ps.size(); i++)
		{
			Parser* p = ps[i];
			STNode* t= 0;
			Result r = p->Parse(_s, t);
			e += r.fail;
			if(r)
			{
				_tree = t;
				return r;
			}
		}
		
		return Failure(e);
	}
};


class ReferenceParser : public Parser
{
	Parser** p;
public:
	ReferenceParser(Parser** _p)
		: p(_p)
	{
	}
	virtual void Reset()
	{
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		return (*p)->Parse(_s, _tree);
	}
};

class TokenParser : public Parser
{
	Parser* p;

	class Stringifier : public TreeVisitor
	{
		string result;
	public:
		bool Visit(STNode* _node, unsigned int _level)
		{
			if(!_node->childs.size())
				result += _node->data;

			return true;
		}
		string GetResult()
		{
			return result;
		}
	};

public:
	TokenParser(Parser* _p)
		: p(_p)
	{
	}
	virtual ~TokenParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		Position start = _s->Where();
		Result r = p->Parse(_s, _tree);
		if(r && _tree)
		{
			Stringifier s;
			PreWalk(_tree, &s);
			delete _tree;
			_tree = new STNode(start, s.GetResult());
			return Success();
		}
		return r;
	}
};

class IgnoreParser : public Parser
{
	Parser* p;

public:
	IgnoreParser(Parser* _p)
		: p(_p)
	{
	}
	virtual ~IgnoreParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		Result r = p->Parse(_s, _tree);
		delete _tree;
		_tree = 0;
		return r;
	}
};

class ClearParser : public Parser
{
	Parser* p;

public:
	ClearParser(Parser* _p)
		: p(_p)
	{
	}
	virtual ~ClearParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		return p->Parse(_s, _tree).Clear();
	}
};

class MemoryParser : public Parser
{
	class Memorizer
	{
		struct Memorization  
		{
			Result result;
			Position newPosition;
			STNode* tree;

			Memorization(const Result& _r, const Position& _p, STNode* _tree)
				: result(_r), newPosition(_p), tree(_tree)
			{}
		};

		map<Position, Memorization> memory;

		STNode* Copy(STNode* _node)
		{
			if(!_node)
				return 0;

			STNode* newNode = new STNode(_node->where, _node->data);
			for(unsigned int i = 0; i < _node->Sons(); i++)
				newNode->AddSon(Copy(_node->Son(i)));
			return newNode;
		}
	public:
		void Reset()
		{
			//Must walk deleting trees as they're copied
			for(map<Position, Memorization>::iterator i = memory.begin(); i != memory.end(); i++)
			{
				delete i->second.tree;
			}

			memory.clear();
		}
		bool IsKnown(const Position& _position)
		{
			return (memory.find(_position) != memory.end());
		}
		bool Remember(const Position& _position, Result& _result, Position& _newPosition, STNode*& _tree)
		{
			map<Position, Memorization>::iterator i = memory.find(_position);
			if (i == memory.end())
				return false;

			_result      = i->second.result;
			_newPosition = i->second.newPosition;
			_tree        = Copy(i->second.tree);
			return true;
		}
		void Memorize(const Position& _position, const Result& _result, const Position& _newPosition, STNode* _tree)
		{
			map<Position, Memorization>::iterator i = memory.find(_position);
			if (i == memory.end())
			{
				memory.insert(pair<Position, Memorization>(_position, Memorization(_result, _newPosition, Copy(_tree))));
			}
		}
	};

	Memorizer memory;
	Parser* p;
public:
	MemoryParser(Parser* _p)
		: p(_p)
	{
	}
	virtual ~MemoryParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		memory.Reset();
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		if(memory.IsKnown(_s->Where()))
		{
			Result r;
			Position n;
			
			memory.Remember(_s->Where(), r, n, _tree);

			if(n != _s->Where())
				_s->Goto(n);
			
			return r;
		}
		
		Position start = _s->Where();

		Result r = p->Parse(_s, _tree);

		memory.Memorize(start, r, _s->Where(), _tree);

		return r;
	}
};



Parser* Char(const Set& _set)		{return new CharParser(_set);}
Parser* Word(const string& _word)	{return new MemoryParser(new WordParser(_word));}
Parser* Empty()						{return new EmptyParser();}
Parser* Any()						{return new AnyParser();}
Parser* EndOfInput()				{return new EndOfInputParser();}

Parser* At		(Parser* _p)						{return new MemoryParser(new CheckParser(_p, true));}
Parser* NotAt	(Parser* _p)						{return new MemoryParser(new CheckParser(_p, false));}
Parser* Optional(Parser* _p)						{return new MemoryParser(new RepeatParser(_p, 0, 1));}
Parser* Star	(Parser* _p)						{return new MemoryParser(new RepeatParser(_p, 0, -1));}
Parser* Plus	(Parser* _p)						{return new MemoryParser(new RepeatParser(_p, 1, -1));}
Parser* Repeat	(int _minN, int _maxN, Parser* _p)	{return new MemoryParser(new RepeatParser(_p, _minN, _maxN));}
Parser* Sequence(unsigned int _number, ...)
{
	va_list arguments;                     
	vector<Parser*> ps;

	va_start(arguments, _number);           
	for(unsigned int i = 0; i < _number; i++)
	{
		ps.push_back(va_arg(arguments, Parser*)); 
	}
	va_end(arguments);

	return new MemoryParser(new SequenceParser(ps));
}
Parser* Choice	(unsigned int _number, ...)
{
	va_list arguments;                     
	vector<Parser*> ps;

	va_start(arguments, _number);           
	for(unsigned int i = 0; i < _number; i++)
	{
		ps.push_back(va_arg(arguments, Parser*)); 
	}
	va_end(arguments);

	return new MemoryParser(new ChoiceParser(ps));
}

Parser* Reference	(Parser** _p)	{return new ReferenceParser(_p);}
Parser* Token		(Parser* _p)	{return new TokenParser(_p);}
Parser* Ignore		(Parser* _p)	{return new IgnoreParser(_p);}
Parser* Clear		(Parser* _p)	{return new ClearParser(_p);}


class NameParser : public Parser
{
	Parser* p;
	string name;
	bool insert;
public:
	NameParser(Parser* _p, const string& _name, bool _insert)
		: p(_p), name(_name), insert(_insert)
	{
	}
	virtual ~NameParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		Position start = _s->Where();

		Result r = p->Parse(_s, _tree);

		if(!r)
			return r;

		if(!_tree)
		{
			if(insert)
				_tree = new STNode(start, name);
			return r;
		}

		if(_tree->HasData())
		{
			if(insert)
			{
				STNode* n = new STNode(_tree->where, name);
				n->AddSon(_tree);
				_tree = n;
			}
		}
		else
			_tree->data = name;

		return r;
	}
};


class RootParser : public Parser
{
	Parser* p;
	int index;
	
public:
	RootParser(Parser* _p, int _index)
		: p(_p), index(_index)
	{
	}
	virtual ~RootParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		Result r = p->Parse(_s, _tree);

		if(!_tree || _tree->HasData())
			return r;

		if(static_cast<unsigned int>((abs(index) - 1)) >= _tree->Sons())
			return r;

		if(index == 0)
			return r;


		int real_index = index > 0 ? index - 1 : (_tree->Sons() + index);
		STNode* son = _tree->Son(real_index);
		_tree->Unlink(son);
		_tree->Merge(son, real_index);
		_tree->data = son->data;
		_tree->where = son->where;
		delete son;

		return r;
	}
};

class FlatParser : public Parser
{
	Parser* p;
	int index;

	class Flatenizer : public TreeVisitor
	{
		STNode* result;
	public:
		Flatenizer(const Position& _originalNodePosition)
			: result(new STNode(_originalNodePosition))
		{
		}
		bool Visit(STNode* _node, unsigned int _level)
		{
			if(_node->HasData())
			{
				result->AddSon(_node);
				return false;
			}

			return true;
		}
		STNode* GetResult()
		{
			return result;
		}
	};

	class Unlinker : public TreeVisitor
	{
	public:
		bool Visit(STNode* _node, unsigned int _level)
		{
			if(_node->HasData())
				return false;

			_node->UnlinkAll();
			delete _node;
			return true;
		}
	};

public:
	FlatParser(Parser* _p, int _index)
		: p(_p), index(_index)
	{
	}
	virtual ~FlatParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		Result r = p->Parse(_s, _tree);

		if(!_tree || _tree->HasData())
			return r;

		if(static_cast<unsigned int>((abs(index) - 1)) >= _tree->Sons())
			return r;

		if(index == 0)
			return r;

		int real_index = index > 0 ? index - 1 : (_tree->Sons() + index);
		STNode* son = _tree->Son(real_index);
		if(son->HasData())
			return r;

		_tree->Unlink(son);

		Flatenizer f(son->where);
		PreWalk(son, &f);
		
		_tree->Merge(f.GetResult(), real_index);

		Unlinker u;
		PostWalk(son, &u);

		return r;
	}
};

class LeftParser : public Parser
{
	Parser* p;

public:
	LeftParser(Parser* _p)
		: p(_p)
	{
	}
	virtual ~LeftParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		Result r = p->Parse(_s, _tree);

		if(!_tree || _tree->HasData())
			return r;

		if(_tree->Sons() % 2 == 0)
			return r;

		for(unsigned int i = 1; i < _tree->Sons(); i += 2)
		{
			if(!_tree->Son(i)->IsLeaf())
				return r;
		}

		for(unsigned int i = 1; i < _tree->Sons(); i += 2)
		{
			STNode* op = _tree->Son(i);
			if(i == 1)
				op->AddSon(_tree->Son(i - 1));
			else
				op->AddSon(_tree->Son(i - 2));
			op->AddSon(_tree->Son(i + 1));
		}

		STNode* old = _tree;
		
		_tree = _tree->Son(_tree->Sons() - 2);
		
		old->UnlinkAll();
		delete old;

		return r;
	}
};

class RightParser : public Parser
{
	Parser* p;

public:
	RightParser(Parser* _p)
		: p(_p)
	{
	}
	virtual ~RightParser()
	{
		delete p;
	}
	virtual void Reset()
	{
		p->Reset();
	}
	virtual Result Parse(Stream* _s, STNode*& _tree)
	{
		Result r = p->Parse(_s, _tree);

		if(!_tree || _tree->HasData())
			return r;

		if(_tree->Sons() % 2 == 0)
			return r;

		for(unsigned int i = 1; i < _tree->Sons(); i += 2)
		{
			if(!_tree->Son(i)->IsLeaf())
				return r;
		}

		for(int i = _tree->Sons() - 2; i > 0; i -= 2)
		{
			STNode* op = _tree->Son(i);
			op->AddSon(_tree->Son(i - 1));

			if(i == _tree->Sons() - 2)
				op->AddSon(_tree->Son(i + 1));
			else
				op->AddSon(_tree->Son(i + 2));
		}

		STNode* old = _tree;

		_tree = _tree->Son(1);

		old->UnlinkAll();
		delete old;

		return r;
	}
};

Parser* Name (const string& _name, bool _insert, Parser* _p){return new NameParser(_p, _name, _insert);}
Parser* Root (int _index, Parser* _p)						{return new RootParser(_p, _index);}
Parser* Flat (int _index, Parser* _p)						{return new FlatParser(_p, _index);}
Parser* Left (Parser* _p)									{return new LeftParser(_p);}
Parser* Right(Parser* _p)									{return new RightParser(_p);}
