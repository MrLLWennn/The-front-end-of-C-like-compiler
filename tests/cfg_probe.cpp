#include "front_end/parser/context_free_grammar.h"

int main()
{
    CFG::ContextFreeGrammar grammar;
    grammar.cal_Vn_Vt();
    grammar.cal_first_and_follow();
    return 0;
}
