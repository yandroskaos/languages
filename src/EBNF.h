#ifndef __EBNF_H__
#define __EBNF_H__

#include "Languages.h"
/**
* @brief Base class for semantic verifiers.
*/
class Semantics
{
public:
	virtual Result Check(STNode* _st) = 0;
};

/**
* @brief Base class for code generators.
*/
class CodeGenerator
{
public:
	virtual Result Generate(STNode* _st, const string& _path) = 0;
};

Parser*			EBNF_Parser();
Semantics*		EBNF_Semantics();
CodeGenerator*	EBNF_CodeGenerator();

#endif
