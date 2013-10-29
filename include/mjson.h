/********************************************************************\

  Name:         mjson.h
  Created by:   Konstantin Olchanski

  Contents:     JSON encoder and decoder

\********************************************************************/

#ifndef _MJSON_H_
#define _MJSON_H_

#include <string>
#include <vector>

#define MJSON_ERROR -1
#define MJSON_NONE   0
#define MJSON_ARRAY  1
#define MJSON_OBJECT 2
#define MJSON_STRING 3
#define MJSON_INT    4
#define MJSON_NUMBER 5
#define MJSON_BOOL   6
#define MJSON_NULL   7

class MJsonNode;

typedef std::vector<std::string> MJsonStringVector;
typedef std::vector<MJsonNode*> MJsonNodeVector;

class MJsonNode {
 protected:
   int type;
   MJsonNodeVector   subnodes;
   MJsonStringVector objectnames;
   std::string     stringvalue;
   int             intvalue;
   double          numbervalue;
   
 public:
   ~MJsonNode(); // dtor

 public: // parser
   static MJsonNode* Parse(const char* jsonstring);
   
 public: // encoder
   std::string Stringify(int flags = 0) const;
   
 public: // public factory constructors
   static MJsonNode* MakeArray();
   static MJsonNode* MakeObject();
   static MJsonNode* MakeString(const char* value);
   static MJsonNode* MakeInt(int value);
   static MJsonNode* MakeNumber(double value);
   static MJsonNode* MakeBool(bool value);
   static MJsonNode* MakeNull();
   static MJsonNode* MakeError(MJsonNode* errornode, const char* errormessage, const char* sin, const char* serror);
   
 public: // public "put" methods
   void AddToArray(MJsonNode* node); /// add node to an array. the array takes ownership of this node
   void AddToObject(const char* name, MJsonNode* node); /// add node to an object. the object takes ownership of this node

 public: // public "get" methods
   int                    GetType() const;   /// get node type: MJSON_xxx
   const MJsonNodeVector* GetArray() const;  /// get array value, NULL if not array, empty array if value is JSON "null"
   const MJsonStringVector* GetObjectNames() const; /// get array of object names, NULL if not object, empty array if value is JSON "null"
   const MJsonNodeVector*   GetObjectNodes() const; /// get array of object subnodes, NULL if not object, empty array if value is JSON "null"
   const MJsonNode*       FindObjectNode(const char* name) const; /// find subnode with given name, NULL if not object, NULL is name not found
   std::string            GetString() const; /// get string value, "" if not string or value is JSON "null"
   int                    GetInt() const;    /// get integer value, 0 if not an integer or value is JSON "null"
   double                 GetNumber() const; /// get number or integer value, 0 if not a number or value is JSON "null"
   bool                   GetBool() const;   /// get boolean value, false if not a boolean or value is JSON "null"
   std::string            GetError() const;  /// get error message from MJSON_ERROR nodes

 public: // public helper and debug methods
   static const char* TypeToString(int type); /// return node type as string
   void Dump(int nest = 0) const; /// dump the subtree to standard output

 private:
   MJsonNode(); // private constructor
};

#endif

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
