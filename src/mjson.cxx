/********************************************************************\

  Name:         mjson.cxx
  Created by:   Konstantin Olchanski

  Contents:     JSON encoder and decoder

\********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "mjson.h"

std::vector<std::string> MJsonNode::GetKeys(const MJsonNodeMap& map) /// helper: get array keys
{
   std::vector<std::string> v;
   for(MJsonNodeMap::const_iterator it = map.begin(); it != map.end(); ++it)
      v.push_back(it->first);
      return v;
}

MJsonNode::~MJsonNode() // dtor
{
   // need to delete everything pointed to by arrayvalue and objectvalue
   assert(!"FIXME!");

   // poison deleted nodes
   type = MJSON_NONE;
}

MJsonNode* MJsonNode::Parse(const char* jsonstring)
{
   return NULL;
}
   
std::string MJsonNode::Stringify(int flags) const
{
   return "";
}
   
MJsonNode* MJsonNode::MakeArray()
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_ARRAY;
   return n;
}

MJsonNode* MJsonNode::MakeObject()
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_OBJECT;
   return n;
}

MJsonNode* MJsonNode::MakeString(const char* value)
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_STRING;
   n->stringvalue = value;
   return n;
}

MJsonNode* MJsonNode::MakeInt(int value)
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_INT;
   n->intvalue = value;
   n->numbervalue = value;
   return n;
}

MJsonNode* MJsonNode::MakeNumber(double value)
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_NUMBER;
   n->numbervalue = value;
   return n;
}

MJsonNode* MJsonNode::MakeBool(bool value)
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_BOOL;
   if (value)
      n->intvalue = 1;
   else
      n->intvalue = 0;
   return n;
}

MJsonNode* MJsonNode::MakeNull()
{
   MJsonNode* n = new MJsonNode();
   n->type = MJSON_NULL;
   return n;
}

int MJsonNode::GetType() const /// get node type: MJSON_xxx
{
   return type;
}

const MJsonNodeVector* MJsonNode::GetArray() const
{
   if (type == MJSON_ARRAY || type == MJSON_NULL)
      return &arrayvalue;
   else
      return NULL;
}

const MJsonNodeMap* MJsonNode::GetObject() const
{
   if (type == MJSON_OBJECT || type == MJSON_NULL)
      return &objectvalue;
   else
      return NULL;
}

std::string MJsonNode::GetString() const
{
   if (type == MJSON_STRING)
      return stringvalue;
   else
      return "";
}

int MJsonNode::GetInt() const
{
   if (type == MJSON_INT)
      return intvalue;
   else
      return 0;
}

double MJsonNode::GetNumber() const
{
   if (type == MJSON_INT)
      return intvalue;
   else if (type == MJSON_NUMBER)
      return numbervalue;
   else
      return 0;
}

bool MJsonNode::GetBool() const /// get boolean value, false if not a boolean or value is JSON "null"
{
   if (type == MJSON_BOOL)
      return (intvalue != 0);
   else
      return false;
}

MJsonNode::MJsonNode() // private constructor
{
   // C++ does not know how to initialize elemental type, we have to do it by hand:
   type = MJSON_NONE;
   intvalue = 0;
   numbervalue = 0;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
