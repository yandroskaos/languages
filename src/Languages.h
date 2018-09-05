/******************************************************************************/
/**
* @file		Languages.h
* @brief	Parser combinators for fast language prototyping.
*
* Library for generating in a fast and easy way LL "PEG" parsers with backtracking.
* Allows to express Type 3 rules easily via parser combinators.
* Type 2 rules are allowed via recursive references.
*/
/******************************************************************************/
#ifndef __LANGUAGES_H__
#define __LANGUAGES_H__

#include <string>
#include <vector>
using namespace std;

/**
* @brief Represents a position into a stream, identified by row and column.
* It is (1, 1) based.
*/
struct Position
{
	unsigned int row;
	unsigned int column;

	Position(unsigned int _row = 0, unsigned int _column = 0);

	bool operator< (const Position& _p) const; //!< row < _p.row or having equal rows, column < _p.column
	bool operator> (const Position& _p) const; //!< row > _p.row or having equal rows, column > _p.column
	bool operator==(const Position& _p) const;
	bool operator!=(const Position& _p) const;
};

/**
* @brief Represents a stream of characters.
*/
class Stream
{
public:
	virtual ~Stream();

	/**
	* @brief Obtains the current character in the stream.
	* This is always valid assuming the stream has at least one character.
	* @return The current character in the stream.
	*/
	virtual char		Get()	const						= 0;
	/**
	* @brief Moves forward the stream so when asked, it will return the next character.
	*/
	virtual void		Next()								= 0;
	/**
	* @brief Indicates if the stream has no more characters.
	* @return True if the stream has ended. False otherwise.
	*/
	virtual bool		AtEnd()	const						= 0;
	/**
	* @brief Indicates where is the head of the stream pointing at.
	* @return Current head position.
	*/
	virtual Position	Where()	const						= 0;
	/**
	* @brief Moves the head to the new indicated position. Needed for memorization.
	* @param _newPosition [in] Where to put the reading head. Must be valid to success.
	* @return True if successful, false otherwise.
	*/
	virtual bool		Goto(const Position& _newPosition)  = 0;
};

/**
* @brief Return a new Stream that is able to read data from memory.
* @param _first [in] Start of the char array.
* @param _last  [in] End of the char array.
* @return A memory stream.
*/
Stream* MemoryStream(const char* _first, const char* _last);
/**
* @brief Return a new Stream that is able to read data from a file.
* @param _fileName [in] Path of the file to read from.
* @return A file stream.
*/
Stream* FileStream	(const char* _fileName);






/**
* @brief Represents a Syntax Tree Node.
*/
struct STNode
{
	Position		where;	//!< Position in the stream where the node is created.
	string			data;	//!< Data associated with this node.
	vector<STNode*>	childs;	//!< Sons of this node.
	
	/**
	* @brief Constructor.
	*/
	STNode(const Position& _where, const string& _data = "");
	/**
	* @brief Destructor. Deletes recursively the sons (entire tree in the end).
	*/
	~STNode();

	/**
	* @brief Indicates if the node has data associated.
	* Aggregation nodes such as sequences starts with its data empty.
	* @return Returns if the node has data associated.
	*/
	bool HasData();
	/**
	* @brief Indicates if the node has children or not. A leaf node does not have children.
	* @return Returns true when the node does not have children.
	*/
	bool IsLeaf();

	/**
	* @brief Given a node, inserts all its children in this node, in the position indicated.
	* Other node son's are unlinked from it as now are children of this node.
	* If node is a leaf, there is no effect.
	* @param _son   [in] Node whose son's are to be inserted into this one.
	* @param _where [in] Index where nodes are to be inserted.
	*/
	void Merge(STNode* _son, unsigned int _where);

	/**
	* @brief Adds a son if son is not null.
	* @param _son   [in] Node to add.
	*/
	void		 AddSon	(STNode* _son);
	/**
	* @brief Obtains a son node.
	* @param _index [in] Index of the son node to obtain (0-based).
	* @return Child node in the corresponding index. 0 if index is invalid.
	*/
	STNode*		 Son	(unsigned int _index);
	/**
	* @brief Number of children this node has.
	* @return Number of children.
	*/
	unsigned int Sons	();

	/**
	* @brief Unlinks a node, which means that the given node, if it is a children, it will be no more.
	* @param _son [in] Node to be unlinked.
	*/
	void Unlink(STNode* _son);
	/**
	* @brief Unlinks all sons.
	*/
	void UnlinkAll();
};

/**
* @brief Interface to implement custom behavior when visiting a Node due to a walking of the tree.
*/
class TreeVisitor
{
public:
	virtual bool Visit(STNode* _node, unsigned int _level) = 0;
};

void PreWalk	(STNode* _root, TreeVisitor* _visitor);  //!< Walks the _root tree in preorder.  _visitor->Visit() is invoker per-node.
void InWalk		(STNode* _root, TreeVisitor* _visitor);  //!< Walks the _root tree in inorder.   _visitor->Visit() is invoker per-node.
void PostWalk	(STNode* _root, TreeVisitor* _visitor);  //!< Walks the _root tree in postorder. _visitor->Visit() is invoker per-node.



/**
* @brief A set of characters
*/
class Set
{
private:
	vector<char> elements;
	
	bool contains(const vector<char>& _set, const char& _element) const;

public:
	Set();        //!< Empty set.
	Set(char _c); //!< Set with one character. Allows implicit casting from char to Set

	Set operator+(const Set& _set) const; //!< Union operation
	Set operator*(const Set& _set) const; //!< Intersection operation
	Set operator-(const Set& _set) const; //!< Difference operation

	bool operator< (const Set& _set) const; //!< Contains operation.
	bool operator> (const Set& _set) const; //!< Is contained operation
	bool operator==(const Set& _set) const;
	bool operator!=(const Set& _set) const;

	string Name(); //!< String representation of the set
};

/**
* @brief Set constructor using a range of chars. Every char in [_begin, _end] will be added to the set.
* @param _begin [in] First char to add
* @param _end   [in] Last char to add
* @return Set built with chars in [_begin, _end].
*/
Set Range		(char _begin, char _end);
/**
* @brief Set constructor using an enumeration of chars. Every char passed as parameter will be added to the set.
* @param _number [in] Number of chars in the variant call to add to the set.
* @return Set built with "_number" chars passed as parameters.
*/
Set Enumeration (unsigned int _number, ...);



/**
* @brief Represents an error in parsing.
* Contains the position of the error and a list of acceptable char's.
* Behaves also as a bool value, meaning by true that there do exist an error (has a not empty list or not default position), 
* and false that there's nothing to report.
*/
struct Error
{
	Position       where;    //!< Position in the stream of the error.
	vector<string> expected; //!< List of acceptable char's at that point.

	/**
	* @brief Constructor.
	* @param _where    [in] Position of the error in the stream.
	* @param _expected [in] Single string containing an acceptable char associated with this error.
	*/
	Error(const string& _expected = "", const Position& _where = Position());

	/**
	* @brief Aggregates errors. "this" will get as position the higher position (the furthest position in the stream), and
	* the associated expected list with the position.
	* If both errors ("this" and _error) are at the same position, expected lists are merged.
	*/
	Error& operator+=(const Error& _error);
	/**
	* @brief Indicates if the is an actual error. There will be if expected list is not empty or position is not default position (0, 0)
	*/
	operator bool();
};

/**
* @brief Result of a Parser. Retrieves both if the parser has succeeded and associated data such as the ST tree in case of success
* and the error in case of failure.
* Also, due to optional parsers (*, +, ?), there maybe an error associated even in the case of success, and this error must be propagated.
* Behaves also as a bool value.
*/
struct Result
{
	bool	match; //!< Success
	Error	fail;  //!< Error

	Result(bool _match = false, const Error& _fail = Error());

	operator bool();    //!< Indicates the success or failure
	Result operator!(); //!< Negates the result

	Result Clear();     //!< Deletes the error if there is any.
};

/**
* @brief Constructs a successful result.
* Can be empty, have an associated ST tree and, optionally, an error.
*/
Result Success(const Error& _fail = Error());
/**
* @brief Constructs a failure result.
* Has an associated error.
*/
Result Failure(const Error& _fail);




/**
* @brief Parser. Given a Stream, recognizes text from the stream and creates the ST.
*/
class Parser
{
public:
	virtual ~Parser();
	/**
	* @brief Resets the parser. Parsers have memoization to speed up backtracks.
	* So after every call to "Parse", the parser should be reset to clear the caches.
	*/
	virtual void   Reset()           = 0;
	/**
	* @brief Do the actual parsing.
	* @param _s [in] Stream to read from.
	* @return Result of parsing. @see Result.
	*/
	virtual Result Parse(Stream* _s, STNode*& _tree) = 0;
};

//Basic Parsers
Parser* Char		(const Set& _set);     //!< Recognizes any char of the set
Parser* Word		(const string& _word); //!< Recognizes the string _word
Parser* Empty		();                    //!< Success always
Parser* Any			();                    //!< Recognizes any char
Parser* EndOfInput	();                    //!< Success if stream is at end of input, fails otherwise

//Combinators
Parser* At			(Parser* _p);                       //!< Success if _p succeeds but no input is consumed.
Parser* NotAt		(Parser* _p);                       //!< Success if _p fails but no input is consumed.
Parser* Optional	(Parser* _p);                       //!< Success always. If _p succeeds, input is consumed, otherwise it is not.
Parser* Star		(Parser* _p);                       //!< Success while _p succeeds. Input is consumed only on _p success.
Parser* Plus		(Parser* _p);                       //!< Success if _p succeeds at least once. Later it behaves as Star conuming input while _p succeeds.
Parser* Repeat		(int _minN, int _maxN, Parser* _p); //!< Success if _p succeeds [_minN, _maxN] times. If _maxN == -1, no upper limit. Consumes until _maxN.
Parser* Sequence	(unsigned int _number, ...);        //!< Success if specified parsers succeeds all in sequence.
Parser* Choice		(unsigned int _number, ...);        //!< Success if at least one parser succeeds.

//Special Parsers
/**
* @brief Given a reference to a parser, it routes every call to this enclosed parser.
* It is useful when declaring mutually recursive parsers.
*/
Parser* Reference	(Parser** _p);
/**
* @brief Tokenizes input. The resulting tree is colapsed into a node with the preorder string of the original tree.
* @param _p      [in] Parser whose tree will be tokenized
*/
Parser* Token		(Parser* _p);
/**
* @brief Ignores input. The resulting tree is discarded.
* @param _p      [in] Parser whose tree will be discarded
*/
Parser* Ignore		(Parser* _p);
/**
* @brief Discards error information. Useful when tokenizing.
* @param _p      [in] Parser whose error will be cleared
*/
Parser* Clear		(Parser* _p);


//Semantic Parsers
/**
Semantic parsers manipulate the resulting ST trees.
To explain theirs effects, will use the following notation:
{}[_a, _b] -> represents an aggregated node with no data which has 2 sons, a tree denoted by _a with the result of parser _a, and the same for _b
{a}[] -> represents a leaf node with data "a".
{x}[_a, _b] -> ST node with both data ("x") and two sons.
tree * semantic operator -> will mean the application of the semantic parser to tree.
*/

/**
* @brief Once parser _p has succeeded, we can name its resulting node to beautify the ST tree.
* If node is a named leaf and _insert is true, the node is inserted as parent of the result tree.
* If node has not data, the name is set as the node data, be _insert true or false.
* Ej:
* {}[_q, _p]    * NameNode("k")         => {"k"}[_q, _p]
* {}[_q, _p]    * NameNode("k", insert) => {"k"}[_q, _p]
* {"r"}[_q, _p] * NameNode("k")         => {"r"}[_q, _p]
* {"r"}[_q, _p] * NameNode("k", insert) => {"k"}[{"r"}[_q, _p]]
*
* @param _name   [in] Parser whose tree will be tokenized
* @param _insert [in] Parser to ignore before trying to match _p
*/
Parser* Name(const string& _name, bool _insert, Parser* _p);
/**
* @brief In an aggregated tree without data, it promotes to root one of the sons, indicated by _index.
* _index is 1-based, and can be negative (-1-based) indicating starting from the end.
* If the elected node is not a leaf, its son are merged in the position it occupied before promoting.
* Ej:
* {}[{"!"}[], _p]           * RootNode(1)  => {"!"}[_p]
* {}[_a, _b, {"x"}[_c, _d]] * RootNode(-1) => {"x"}[_a, _b, _c, _d]
*
* @param _index [in] Index of the node (1/-1-based).
*/
Parser* Root(int _index, Parser* _p);
/**
* @brief In an aggregated tree, it merges all the children one of the sons, indicated by _index, as sons in the tree.
* _index is 1-based, and can be negative (-1-based) indicating starting from the end.
* This is useful when having a node which one of its son are also aggregated and want the ST tree to have only on level.
* These trees are generally the result of parsers like Sequence(_p, Star(_p)) and this parser "flats" [_p, [_p, ...]] into [_p, _p, ...].
* Ej:
* {}[_a, [_b, _c]]       * FlatNode(2)  => {}[_a, _b, _c]
* {"x"}[_a, [_b, _c]]    * FlatNode(-1) => {"x"}[_a, _b, _c]
* {}[_a, [[_b, _c], _d]] * FlatNode(2)  => {}[_a, [_b, _c], _d]
*
* @param _index [in] Index of the node (1/-1-based).
*/
Parser* Flat(int _index, Parser* _p);

/**
* @brief Given a flat list in the form of [a, op, b, op, b], it generates a left associative tree.
* Ej:
* {}[_a, _op, _b, _op, _c] * Left => {_op}[{_op}[_a, _b], _c]
*/
Parser* Left(Parser* _p);

/**
* @brief Given a flat list in the form of [a, op, b, op, b], it generates a right associative tree.
* Ej:
* {}[_a, _op, _b, _op, _c] * Right => {_op}[_a, {_op}[_b, _c]]
*/
Parser* Right(Parser* _p);

#endif
