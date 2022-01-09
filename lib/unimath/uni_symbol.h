#ifndef LATEX_UNI_SYMBOL_H
#define LATEX_UNI_SYMBOL_H

#include "utils/utils.h"
#include "utils/enums.h"
#include "utils/exceptions.h"

namespace tex {

struct Symbol {
private:
  static const Symbol _symbols[];
  static const i32 _count;

public:
  const c32 unicode;
  const u32 flag;
  const char* name;

  AtomType type() const;

  LimitsType limitsType() const;

  bool isVariable() const;

  /**
   * Get the symbol from the given name.
   *
   * @param name the name of the symbol
   * @return the symbol of the given name or null if not found
   */
  static const Symbol* get(const char* name);
};

}

#endif //LATEX_UNI_SYMBOL_H