/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.stevestreeting.com/ogre/

Copyright (c) 2000-2005 The OGRE Team
Also see acknowledgements in Readme.html

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/gpl.html.
-----------------------------------------------------------------------------
*/


#ifndef __Compiler2Pass_H__
#define __Compiler2Pass_H__

#include <vector>
#include "OgrePrerequisites.h"

namespace Ogre {

    /** Compiler2Pass is a generic 2 pass compiler/assembler
    @remarks
	    provides a tokenizer in pass 1 and relies on the subclass to provide the virtual method for pass 2

	    PASS 1 - tokenize source: this is a simple brute force lexical scanner/analyzer that also parses
			    the formed token for proper semantics and context in one pass
			    it uses top down (recursive descent) ruling based on Backus - Naur Form (BNF) notation for semantic
			    checking.

			    During Pass1, if a terminal token is identified as having an action then that action gets triggered
			    when the next terminal token is uncountered that has an action.

	    PASS 2 - generate application specific instructions ie native instructions based on the tokens in the instruction container.

    @par
	    this class must be subclassed with the subclass providing some implementation details for Pass 2.  The subclass
	    is responsible for setting up the token libraries along with defining the language syntax and handling
        token actions during the second pass.

    @par
        The sub class normally supplies a simplified BNF text description in its constructor prior to doing any parsing/tokenizing of source.
        The simplified BNF text description defines the language syntax and rule structure.
        The meta-symbols used in the BNF text description are:
    @par
        ::=  meaning "is defined as". "::=" starts the definition of a rule.  The left side of ::= must contain an <identifier>
    @par
        <>   angle brackets are used to surround syntax rule names. A syntax rule name is also called a non-terminal in that
             it does not generate a terminal token in the instruction container for pass 2 processing.
    @par
        |    meaning "or". if the item on the left of the | fails then the item on the right is tested.
             Example: <true_false> ::= 'true' | 'false';
             whitespace is used to imply AND operation between left and right items.
             Example: <terrain_shadaws> ::= 'terrain_shadows' <true_false>
             the 'terrain_shadows' terminal token must be found and <true_false> rule must pass in order for <terrain_shadows> rule
             to pass.
    @par
        []   optional rule identifier is enclosed in meta symbols [ and ].
             Note that only one identifier or terminal token can take [] modifier.
    @par
        {}   repetitive identifier (zero or more times) is enclosed in meta symbols { and }
             Note that only one identifier or terminal token can take {} modifier.
    @par
        ''   terminal tokens are surrounded by single quotes.  A terminal token is always one or more characters.
             For example: 'Colour' defines a character sequence that must be matched in whole.  Note that matching is case
             sensitive.
    @par
        @    turn on single character scanning and don't skip white space.
             Mainly used for label processing that allow white space.
             Example: '@ ' prevents the white space between the quotes from being skipped
    @par
        -''  no terminal token is generated when a - precedes the first single quote but the text in between the quotes is still
             tested against the characters in the source being parsed.
    @par
        (?! ) negative lookahead (not test) inspired by Perl 5. Scans ahead for a non-terminal or terminal expression
             that should fail in order to make the rule production pass.
             Does not generate a token or advance the cursur.  If the lookahead result fails ie token is found,
             then the current rule fails and rollback occurs.  Mainly used to solve multiple contexts of a token.
             An Example of where not test is used to solve multiple contexts:

             <rule>       ::=  <identifier>  "::="  <expression>\n
             <expression> ::=  <and_term> { <or_term> }\n
             <or_term>    ::=  "|" <and_term>\n
             <and_term>   ::=  <term> { <term> }\n
             <term>       ::=  <identifier_right> | <terminal_symbol> | <repeat_expression> | <optional_expression>\n
             <identifier_right> ::= <identifier> (?!"::=")

             <identiefier> appears on both sides of the ::= so (?!"::=") test to make sure that ::= is not on the
             right which would indicate that a new rule was being formed.

             Works on both terminals and non-terminals.
             Note: lookahead failure causes the whole rule to fail and rollback to occur

    @par
        <#name> # indicates that a numerical value is to be parsed to form a terminal token.  Name is optional and is just a descriptor
             to help with understanding what the value will be used for.
             Example: <Colour> ::= <#red> <#green> <#blue>

    @par
        ()   parentheses enclose a set of characters that can be used to generate a user identifier. for example:
             (0123456789) matches a single character found in that set.
             An example of a user identifier:

    @par
             <Label> ::= <Character> {<Character>}\n
             <Character> ::= (abcdefghijklmnopqrstuvwxyz)

             This will generate a rule that accepts one or more lowercase letters to make up the Label.  The User identifier
             stops collecting the characters into a string when a match cannot be found in the rule.

    @par
        (! ) if the first character in the set is a ! then any input character not found in the set will be
             accepted.
             An example:

             <Label> ::= <AnyCharacter_NoLineBreak> {<AnyCharacter_NoLineBreak>}\n
             <AnyCharacter_NoLineBreak> ::= (!\n\r)

             any character but \n or \r is accepted in the input.

    @par
        :   Insert the terminal token on the left before the next terminal token on the right if the next terminal token on right parses.
             Usefull for when terminal tokens don't have a definate text state but only context state based on another terminal or character token.
             An example:

             <Last_Resort> ::= 'external_command' : <Special_Label>\n
             <Special_Label> ::= (!\n\r\t)

             In the example, <Last_Resort> gets processed when all other rules fail to parse.
             if <Special_Label> parses (reads in any character but \n\r\t) then the terminal token 'external_command'
             is inserted prior to the Special_Label for pass 2 processing.  'external_command' does not have have an explicit text
             representation but based on the context of no other rules matching and <Special_Label> parsing, 'external_command' is
             considered parsed.
    */
    class _OgreExport Compiler2Pass
    {

    protected:

	    // BNF operation types
	    enum OperationType {otUNKNOWN, otRULE, otAND, otOR, otOPTIONAL,
                            otREPEAT, otDATA, otNOT_TEST, otINSERT_TOKEN, otEND};

	    /** structure used to build rule paths

	    */
	    struct TokenRule
        {
		    OperationType operation;
		    size_t tokenID;

            TokenRule(void) : operation(otUNKNOWN), tokenID(0) {}
            TokenRule(const OperationType ot, const size_t token)
                : operation(ot), tokenID(token) {}
	    };

	    typedef std::vector<TokenRule> TokenRuleContainer;
	    typedef TokenRuleContainer::iterator TokenRuleIterator;

        static const size_t SystemTokenBase = 1000;
        enum SystemRuleToken {
            _no_token_ = SystemTokenBase,
            _character_,
            _value_,
            _no_space_skip_
        };

	    enum BNF_ID {BNF_UNKOWN = 0,
            BNF_SYNTAX, BNF_RULE, BNF_IDENTIFIER, BNF_IDENTIFIER_RIGHT, BNF_IDENTIFIER_CHARACTERS, BNF_ID_BEGIN, BNF_ID_END,
            BNF_CONSTANT_BEGIN, BNF_SET_RULE, BNF_EXPRESSION,
            BNF_AND_TERM, BNF_OR_TERM, BNF_TERM, BNF_TERM_ID, BNF_CONSTANT, BNF_OR, BNF_TERMINAL_SYMBOL, BNF_TERMINAL_START,
            BNF_REPEAT_EXPRESSION, BNF_REPEAT_BEGIN, BNF_REPEAT_END, BNF_SET, BNF_SET_BEGIN, BNF_SET_END,
            BNF_NOT_TEST, BNF_NOT_TEST_BEGIN, BNF_CONDITIONAL_TOKEN_INSERT, BNF_OPTIONAL_EXPRESSION,
            BNF_NOT_EXPRESSION, BNF_NOT_CHK,
            BNF_OPTIONAL_BEGIN, BNF_OPTIONAL_END, BNF_NO_TOKEN_START, BNF_SINGLEQUOTE, BNF_SINGLE_QUOTE_EXC, BNF_SET_END_EXC,
            BNF_ANY_CHARACTER, BNF_SPECIAL_CHARACTERS1,
            BNF_SPECIAL_CHARACTERS2, BNF_WHITE_SPACE_CHK,

            BNF_LETTER, BNF_LETTER_DIGIT, BNF_DIGIT, BNF_WHITE_SPACE,
            BNF_ALPHA_SET, BNF_NUMBER_SET, BNF_SPECIAL_CHARACTER_SET1,
			BNF_SPECIAL_CHARACTER_SET2, BNF_SPECIAL_CHARACTER_SET3, BNF_NOT_CHARS
        };


	    /** structure used to build lexeme Type library */
	    struct LexemeTokenDef
        {
	        size_t ID;					/// Token ID which is the index into the Lexeme Token Definition Container
            bool hasAction;            /// has an action associated with it. only applicable to terminal tokens
            bool isNonTerminal;        /// if true then token is non-terminal
	        size_t ruleID;				/// index into Rule database for non-terminal token rulepath and lexeme
	        bool isCaseSensitive;        /// if true use case sensitivity when comparing lexeme to source
            String lexeme;             /// text representation of token or valid characters for label parsing

            LexemeTokenDef(void) : ID(0), hasAction(false), isNonTerminal(false), ruleID(0), isCaseSensitive(false) {}
            LexemeTokenDef( const size_t ID, const String& lexeme, const bool hasAction = false, const bool caseSensitive = false )
                : ID(ID)
                , hasAction(hasAction)
                , isNonTerminal(false)
                , ruleID(0)
                , isCaseSensitive(caseSensitive)
                , lexeme(lexeme)
            {
            }

	    };

        typedef std::vector<LexemeTokenDef> LexemeTokenDefContainer;
        typedef LexemeTokenDefContainer::iterator LexemeTokenDefIterator;

        typedef std::map<std::string, size_t> LexemeTokenMap;
        typedef LexemeTokenMap::iterator TokenKeyIterator;
        /// map used to lookup client token based on previously defined lexeme


	    /** structure for Token instructions that are constructed during first pass*/
	    struct TokenInst
        {
	    size_t NTTRuleID;			/// Non-Terminal Token Rule ID that generated Token
	    size_t tokenID;					/// expected Token ID. Could be UNKNOWN if valid token was not found.
	    size_t line;				/// line number in source code where Token was found
	    size_t pos;				/// Character position in source where Token was found
        bool found;                /// is true if expected token was found
	    };

	    typedef std::vector<TokenInst> TokenInstContainer;
	    typedef TokenInstContainer::iterator TokenInstIterator;

        // token que, definitions, rules
        struct TokenState
        {
            TokenInstContainer       tokenQue;
            LexemeTokenDefContainer  lexemeTokenDefinitions;
	        TokenRuleContainer       rootRulePath;
            LexemeTokenMap           lexemeTokenMap;
        };

        TokenState* mClientTokenState;

	    /// Active token que, definitions, rules currntly being used by parser
        TokenState* mActiveTokenState;
        /// the location within the token instruction container where pass 2 is
        mutable size_t mPass2TokenQuePosition;
        /** the que position of the previous token that had an action.
            A token's action is fired on the next token having an action.
        */
        size_t mPreviousActionQuePosition;
        /** the que position for the next token that has an action.
        */
        size_t mNextActionQuePosition;

	    /// pointer to the source to be compiled
	    const String* mSource;
	    /// name of the source to be compiled
	    String mSourceName;
	    size_t mEndOfSource;

	    size_t mCurrentLine; /// current line number in source being tokenized
        size_t mCharPos;     /// position in current line in source being tokenized
        size_t mErrorCharPos; /// character position in source where last error occured

	    /// storage container for constants defined in source
        /// container uses Token index as a key associated with a float constant
	    std::map<size_t, float> mConstants;
	    /// storage container for string labels defined in source
        /// container uses Token index as a key associated with a label
        std::map<size_t, String> mLabels;
        /// flag indicates when a label is being parsed.
        /// It gets set false when a terminal token not of _character_ is encountered
        bool mLabelIsActive;
        /// the key of the active label being built during pass 1.
        /// a new key is calculated when mLabelIsActive switches from false to true
        size_t mActiveLabelKey;
        /// flag being true indicates that spaces are not to be skipped
        /// automatically gets set to false when mLabelIsActive goes to false
        bool mNoSpaceSkip;
        /// if flag is true then next terminal token is not added to token que if found
        /// but does effect rule path flow
        bool mNoTerminalToken;
        /// TokenID to insert if next rule finds a terminal token
        /// if zero then no token inserted
        size_t mInsertTokenID;

	    /// Active Contexts pattern used in pass 1 to determine which tokens are valid for a certain context
	    uint mActiveContexts;

	    /** perform pass 1 of compile process
		    scans source for lexemes that can be tokenized and then
		    performs general semantic and context verification on each lexeme before it is tokenized.
		    A tokenized instruction list is built to be used by Pass 2.
            A rule path can trigger Pass 2 execution if enough tokens have been generated in Pass 1.
            Pass 1 will then pass control to pass 2 temporarily until the current tokens have been consumed.

	    */
	    bool doPass1();

	    /** performs Pass 2 of compile process which is execution of the tokens
	    @remark
		    Pass 2 takes the token instructions generated in Pass 1 and
		    builds the application specific instructions along with verifying
		    symantic and context rules that could not be checked in Pass 1.
        @par
            Pass 2 execution consumes tokens and moves the Pass 2 token instruction position towards the end
            of the token container.  Token execution can insert new tokens into the token container.
	    */
	    bool doPass2();

        /** execute the action associated with the token pointed to by the Pass 2 token instruction position.
        @remarks
            Its upto the child class to implement how it will associate a token key with and action.
            Actions should be placed at positions withing the BNF grammer (instruction que) that indicate
            enough tokens exist for pass 2 processing to take place.
        */
        virtual void executeTokenAction(const size_t tokenID) = 0;
        /** setup client token definitions.  Gets called when BNF grammer is being setup.
        */
        virtual void setupTokenDefinitions(void) = 0;
        /** Gets the next token from the instruction que.
        @remarks
            If an unkown token is found then an exception is raised but
            the instruction pointer is still moved passed the unknown token.  The subclass should catch the exception,
            provide an error message, and attempt recovery.

        @param expectedTokenID if greater than 0 then an exception is raised if tokenID does not match.
        */
        const TokenInst& getNextToken(const size_t expectedTokenID = 0) const
        {
            skipToken();
            return getCurrentToken(expectedTokenID);
        }
        /** Gets the current token from the instruction que.
        @remarks
            If an unkown token is found then an exception is raised.
            The subclass should catch the exception, provide an error message, and attempt recovery.

        @param expectedTokenID if greater than 0 then an exception is raised if tokenID does not match.

        */
        const TokenInst& getCurrentToken(const size_t expectedTokenID = 0) const;
        /** If a next token instruction exist then test if its token ID matches.
        @remarks
            This method is usefull for peeking ahead during pass 2 to see if a certain
            token exists.
        @param expectedTokenID is the ID of the token to match.
        */
        bool testNextTokenID(const size_t expectedTokenID) const;

        /** If a current token instruction exist then test if its token ID matches.
        @param expectedTokenID is the ID of the token to match.
        */
        bool testCurrentTokenID(const size_t expectedTokenID) const
        {
            return mActiveTokenState->tokenQue[mPass2TokenQuePosition].tokenID == expectedTokenID;
        }
        /** skip to the next token in the pass2 queue.
        */
        void skipToken(void) const;
        /** go back to the previous token in the pass2 queue.
        */
        void replaceToken(void);
        /** Gets the next token's associated floating point value in the instruction que that was parsed from the
            text source.  If an unkown token is found or no associated value was found then an exception is raised but
            the instruction pointer is still moved passed the unknown token.  The subclass should catch the exception,
            provide an error message, and attempt recovery.
        */
        float getNextTokenValue(void) const
        {
            skipToken();
            return getCurrentTokenValue();
        }
        /** Gets the current token's associated floating point value in the instruction que that was parsed from the
            text source.
        @remarks
            If an unkown token is found or no associated value was found then an exception is raised.
            The subclass should catch the exception, provide an error message, and attempt recovery.
        */
        float getCurrentTokenValue(void) const;
        /** Gets the next token's associated text label in the instruction que that was parsed from the
            text source.
        @remarks
            If an unkown token is found or no associated label was found then an exception is raised but
            the instruction pointer is still moved passed the unknown token.  The subclass should catch the exception,
            provide an error message, and attempt recovery.
        */
        const String& getNextTokenLabel(void) const
        {
            skipToken();
            return getCurrentTokenLabel();
        }
        /** Gets the next token's associated text label in the instruction que that was parsed from the
            text source.  If an unkown token is found or no associated label was found then an exception is raised.
            The subclass should catch the exception, provide an error message, and attempt recovery.
        */
        const String& getCurrentTokenLabel(void) const;
        /** Get the next token's ID value.
        */
        size_t getNextTokenID(void) const { return getNextToken().tokenID; }
        /** Get the current token's ID value.
        */
        size_t getCurrentTokenID(void) const { return getCurrentToken().tokenID; }
        /** Get the next token's lexeme string.  Handy when you don't want the ID but want the string
            representation.
        */
        const String& getNextTokenLexeme(void) const
        {
            skipToken();
            return getCurrentTokenLexeme();
        }
        /** Get the current token's lexeme string.  Handy when you don't want the ID but want the string
            representation.
        */
        const String& getCurrentTokenLexeme(void) const;
        /** Gets the number of tokens waiting in the instruction que that need to be processed by an token action in pass 2.
        */
        size_t getPass2TokenQueCount(void) const;
        /** Get the number of tokens not processed by action token.
            Client Actions should use this method to retreive the number of parameters(tokens)
            remaining to be processed in the action.
        */
        size_t getRemainingTokensForAction(void) const;
        /** Manualy set the Pass2 Token que position.
        @remarks
            This method will also set the position of the next token in the pass2 que that
            has an action ensuring that getRemainingTokensForAction works currectly.
            This method is useful for when the token que must be reprocessed after
            pass1 and the position in the que must be changed so that an action will be triggered.
        @param pos is the new position within the Pass2 que
        @param activateAction if set true and the token at the new position has an action then the
            action is activated.
        */
        void setPass2TokenQuePosition(size_t pos, const bool activateAction = false);
        /** Get the current position in the Pass2 Token Que.
        */
        size_t getPass2TokenQuePosition(void) const { return mPass2TokenQuePosition; }
        /** Set the position of the next token action in the Pass2 Token Que.
        @remarks
            If the position is not within the que or there is no action associated with
            the token at the position in the que then NextActionQuePosition is not set.
        @param pos is the position in the Pass2 Token Que where the next action is.
        @param search if true then the que is searched from pos until an action is found.
            If the end of the que is reached and no action has been found then NextActionQuePosition
            is set to the end of the que and false is returned.
        */
        bool setNextActionQuePosition(size_t pos, const bool search = false);
        /** Add a lexeme token association.
        @remarks
            The backend compiler uses the associations between lexeme and token when
            building the rule base from the BNF script so all associations must  be done
            prior to compiling a source.
        @param lexeme is the name of the token and use when parsing the source to determin a match for a token.
        @param token is the ID associated with the lexeme
        @param hasAction must be set true if the client wants an action triggered when this token is generated
        @param caseSensitive should be set true if lexeme match should use case sensitivity
        */
        void addLexemeToken(const String& lexeme, const size_t token, const bool hasAction = false, const bool caseSensitive = false);

        /** Sets up the parser rules for the client based on the BNF Grammer text passed in.
        @remarks
            Raises an exception if the grammer did not compile successfully.  This method gets called
            when a call to compile occurs and no compiled BNF grammer exists, otherwise nothing will happen since the compiler has no rules to work
            with.  The grammer only needs to be set once during the lifetime of the compiler unless the
            grammer changes.
        @note
            BNF Grammer rules are cached once the BNF grammer source is compiled.
            The client should never have to call this method directly.
        */
        void setClientBNFGrammer(void);



        /// find the eol charater
	    void findEOL();

	    /** check to see if the text at the present position in the source is a numerical constant
	    @param fvalue is a reference that will receive the float value that is in the source
	    @param charsize reference to receive number of characters that make of the value in the source
	    @return
		    true if characters form a valid float representation
		    false if a number value could not be extracted
	    */
	    bool isFloatValue(float& fvalue, size_t& charsize) const;

        /** Check if source at current position is supposed to be a user defined character label.
        A new label is processed when previous operation was not _character_ otherwise the processed
        character (if match was found) is added to the current label.  This allows _character_ operations
        to be chained together to form a crude regular expression to build a label.
	    @param rulepathIDX index into rule path database of token to validate.
	    @return
		    true if token was found for character label.
        */
        bool isCharacterLabel(const size_t rulepathIDX);
	    /** check to see if the text is in the lexeme text library
	    @param lexeme points to begining of text where a lexem token might exist
	    @param caseSensitive set to true if match should be case sensitive
	    @return
		    true if a matching token could be found in the token type library
		    false if could not be tokenized
	    */
	    bool isLexemeMatch(const String& lexeme, const bool caseSensitive) const;
	    /// position to the next possible valid sysmbol
	    bool positionToNextLexeme();
	    /** process input source text using rulepath to determine allowed tokens
	    @remarks
		    the method is reentrant and recursive
		    if a non-terminal token is encountered in the current rule path then the method is
		    called using the new rule path referenced by the non-terminal token
		    Tokens can have the following operation states which effects the flow path of the rule
			    RULE: defines a rule path for the non-terminal token
			    AND: the token is required for the rule to pass
			    OR: if the previous tokens failed then try these ones
			    OPTIONAL: the token is optional and does not cause the rule to fail if the token is not found
			    REPEAT: the token is required but there can be more than one in a sequence
                DATA: Used by a previous token ie for character sets
                NOTTEST: performs negative lookahead ie make sure the next token is not of a certain type
			    END: end of the rule path - the method returns the succuss of the rule

	    @param rulepathIDX index into an array of Token Rules that define a rule path to be processed
	    @return
		    true if rule passed - all required tokens found
		    false if one or more tokens required to complete the rule were not found
	    */
	    bool processRulePath( size_t rulepathIDX);


	    /** setup ActiveContexts - should be called by subclass to setup initial language contexts
        */
	    void setActiveContexts(const uint contexts){ mActiveContexts = contexts; }

	    /// comment specifiers are hard coded
	    void skipComments();

	    /// find end of line marker and move past it
	    void skipEOL();

	    /// skip all the white space which includes spaces and tabs
	    void skipWhiteSpace();


	    /** check if current position in source has the lexeme text equivalent to the TokenID
	    @param rulepathIDX index into rule path database of token to validate
	    @param activeRuleID index of non-terminal rule that generated the token
	    @return
		    true if token was found
		    false if token lexeme text does not match the source text
		    if token is non-terminal then processRulePath is called
	    */
	    bool ValidateToken(const size_t rulepathIDX, const size_t activeRuleID);

	    /** scan through all the rules and initialize token definition with index to rules for non-terminal tokens.
            Gets called when internal grammer is being verified or after client grammer has been parsed.
        @param grammerName is the name of the grammer the token rules are for
	    */
	    void verifyTokenRuleLinks(const String& grammerName);
	    /** Checks the last token instruction and if it has an action then it triggers the action of the previously
            found token having an action.
	    */
	    void checkTokenActionTrigger(void);
	    /** Get the text representation of the rule path.  This is a good way to way to visually verify
	    that the BNF grammer did compile correctly.
	    @param ruleID is the index into the rule path.
	    @param level is the number of levels a non-terminal will expand to. Defaults to 0 if not set which
            will cause non-terminals to not expand.
	    */
	    String getBNFGrammerTextFromRulePath(size_t ruleID, const size_t level = 0);


    private:
        // used for interpreting BNF script
        // keep it as static so that only one structure is created
        // no matter how many times this class is instantiated.
        static TokenState mBNFTokenState;
        // maintain a map of BNF grammer
        typedef std::map<String, TokenState> TokenStateContainer;
        static TokenStateContainer mClientTokenStates;
        /// if a previous token action was setup then activate it now
        void activatePreviousTokenAction(void);
        /// initialize token definitions and rule paths
        void initBNFCompiler(void);
        /// Convert BNF grammer token que created in pass 1 into a BNF rule path
        void buildClientBNFRulePaths(void);
        /// modify the last rule in the container. An end operation is added to the rule path.
        void modifyLastRule(const OperationType pendingRuleOp, const size_t tokenID);
        /** get the token ID for a lexeme in the client state. If the lexeme is not found then
            it is added to the map and definition container and a new tokenID created.
        @return the ID of the token.
        */
        size_t getClientLexemeTokenID(const String& lexeme, const bool isCaseSensitive = false);
        /// Extract a Non Terminal identifier from the token que
        void extractNonTerminal(const OperationType pendingRuleOp);
        /// Extract a Terminal lexeme from the token que and add to current rule expression
        void extractTerminal(const OperationType pendingRuleOp, const bool notoken = false);
        /// Extract a set from the token que and add to current rule expression
        void extractSet(const OperationType pendingRuleOp);
        /// Extract a numeric constant from the token que and add it to the current rule expression
        void extractNumericConstant(const OperationType pendingRuleOp);
        /// changes previous terminal token rule into a conditional terminal token insert rule
        void setConditionalTokenInsert(void);
        /// get the lexem text of a rule.
        String getLexemeText(size_t& ruleID, const size_t level = 0);

    public:

	    /// constructor
	    Compiler2Pass();
        virtual ~Compiler2Pass() {}

	    /** compile the source - performs 2 passes.
		    First pass is to tokinize, check semantics and context.
		    The second pass is performed by using tokens to look up function implementors and executing
            them which convert tokens to application specific instructions.
	    @remark
		    Pass 2 only gets executed if Pass 1 has built enough tokens to complete a rule path and found no errors
	    @param source a pointer to the source text to be compiled
	    @return
		    true if Pass 1 and Pass 2 are successfull
		    false if any errors occur in Pass 1 or Pass 2
	    */
	    bool compile(const String& source, const String& sourceName);
        /** gets BNF Grammer.  Gets called when BNF grammer has to be compiled for the first time.
        */
        virtual const String& getClientBNFGrammer(void) = 0;

        /** get the name of the BNF grammer.
        */
        virtual const String& getClientGrammerName(void) const = 0;

    };

}

#endif

