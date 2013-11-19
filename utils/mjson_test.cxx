#include <stdio.h>
#include <unistd.h>
#include "mjson.h"

int main(int argc, char* argv[])
{
   std::string sin;

   while (1) {
      char buf[100];
      int rd = read(0, buf, sizeof(buf)-1);
      if (rd <= 0)
         break;
      buf[rd] = 0;
      sin += buf;
   }

   //printf("Input string:\n");
   //printf("%s\n", sin.c_str());

   MJsonNode* n = MJsonNode::Parse(sin.c_str());
   if (n==NULL) {
      printf("Parse returned NULL!\n");
      return 1;
   }
   n->Dump();
   std::string sout = n->Stringify();
   printf("Output JSON:\n");
   printf("%s\n", sout.c_str());
   delete n;
   return 0;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

