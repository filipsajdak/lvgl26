# The fix to try: the arrow and the star of lv::held say that the precondition rules out a null tree.
p = "include/lv/one_of.hpp"
s = open(p).read()
arrow_old = "    pre(has_value())\n  {\n    return find_(slot_);\n  }"
arrow_new = ("    pre(has_value())\n  {\n    mounted<Tree>* tree = find_(slot_);\n"
             "    if (tree == nullptr) std::unreachable();  // ruled out by the precondition\n    return tree;\n  }")
star_old = "    pre(has_value())\n  {\n    return *find_(slot_);\n  }"
star_new = ("    pre(has_value())\n  {\n    mounted<Tree>* tree = find_(slot_);\n"
            "    if (tree == nullptr) std::unreachable();  // ruled out by the precondition\n    return *tree;\n  }")
assert arrow_old in s and star_old in s
s = s.replace(arrow_old, arrow_new).replace(star_old, star_new)
s = s.replace("#include <type_traits>\n", "#include <type_traits>\n#include <utility>\n", 1)
open(p, "w").write(s)
print("patched")
