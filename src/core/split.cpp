#include "core/split.h"

#include "box/box_group.h"
#include "utils/log.h"

using namespace std;
using namespace tex;

#ifdef HAVE_LOG

static void printBox(const sptr<Box>& b, int dep, vector<bool>& lines) {
  __print("%-4d", dep);
  if (lines.size() < dep + 1) lines.resize(dep + 1, false);

  for (int i = 0; i < dep - 1; i++) {
    if (lines[i]) {
      __print("    ");
    } else {
      __print(" │  ");
    }
  }

  if (dep > 0) {
    if (lines[dep - 1]) {
      __print(" └──");
    } else {
      __print(" ├──");
    }
  }

  if (b == nullptr) {
    __print(ANSI_COLOR_RED
            " NULL\n");
    return;
  }

  const vector<sptr<Box>>& children = b->descendants();
  const auto size = children.size();
  const string& str = b->toString();
  if (size > 0) {
    __print(ANSI_COLOR_CYAN
            " %s\n"
            ANSI_RESET, str.c_str());
  } else {
    __print(" %s\n", str.c_str());
  }

  for (size_t i = 0; i < size; i++) {
    lines[dep] = i == size - 1;
    printBox(children[i], dep + 1, lines);
  }
}

void tex::printBox(const sptr<Box>& box) {
  vector<bool> lines;
  ::printBox(box, 0, lines);
  __print("\n");
}

#endif  // HAVE_LOG

sptr<Box> BoxSplitter::split(const sptr<Box>& b, float width, float lineSpace) {
  auto h = dynamic_pointer_cast<HBox>(b);
  sptr<Box> box;
  if (h != nullptr) {
    auto box = split(h, width, lineSpace);
#ifdef HAVE_LOG
    if (box != b) {
      __print("[BEFORE SPLIT]:\n");
      printBox(b);
      __print("[AFTER SPLIT]:\n");
      printBox(box);
    } else {
      __print("[BOX TREE]:\n");
      printBox(box);
    }
#endif
    return box;
  }
#ifdef HAVE_LOG
  __print("[BOX TREE]:\n");
  printBox(b);
#endif
  return b;
}

sptr<Box> BoxSplitter::split(const sptr<HBox>& hb, float width, float lineSpace) {
  if (width == 0 || hb->_width <= width) return hb;

  auto* vbox = new VBox();
  sptr<HBox> first, second;
  stack<Position> positions;
  sptr<HBox> hbox = hb;

  while (hbox->_width > width && canBreak(positions, hbox, width) != hbox->_width) {
    Position pos = positions.top();
    positions.pop();
    auto hboxes = pos._box->split(pos._index - 1);
    first = hboxes.first;
    second = hboxes.second;
    while (!positions.empty()) {
      pos = positions.top();
      positions.pop();
      hboxes = pos._box->splitRemove(pos._index);
      hboxes.first->add(first);
      hboxes.second->add(0, second);
      first = hboxes.first;
      second = hboxes.second;
    }
    vbox->add(first, lineSpace);
    hbox = second;
  }

  if (second != nullptr) {
    vbox->add(second, lineSpace);
    return sptr<Box>(vbox);
  }

  return hbox;
}

float BoxSplitter::canBreak(stack<Position>& s, const sptr<HBox>& hbox, const float width) {
  const vector<sptr<Box>>& children = hbox->_children;
  const int count = children.size();
  // Cumulative width
  auto* cumWidth = new float[count + 1]();
  cumWidth[0] = 0;
  for (int i = 0; i < count; i++) {
    auto box = children[i];
    cumWidth[i + 1] = cumWidth[i] + box->_width;
    if (cumWidth[i + 1] <= width) continue;
    int pos = getBreakPosition(hbox, i);
    auto h = dynamic_pointer_cast<HBox>(box);
    if (h != nullptr) {
      stack<Position> sub;
      float w = canBreak(sub, h, width - cumWidth[i]);
      if (w != box->_width && (cumWidth[i] + w <= width || pos == -1)) {
        s.push(Position(i - 1, hbox));
        // add to stack
        vector<Position> p;
        while (!sub.empty()) {
          p.push_back(sub.top());
          sub.pop();
        }
        for (auto it = p.rbegin(); it != p.rend(); it++) s.push(*it);
        // release cum-width
        float x = cumWidth[i] + w;
        delete[] cumWidth;
        return x;
      }
    }

    if (pos != -1) {
      s.push(Position(pos, hbox));
      float x = cumWidth[pos];
      delete[] cumWidth;
      return x;
    }
  }

  delete[] cumWidth;
  return hbox->_width;
}

int BoxSplitter::getBreakPosition(const sptr<HBox>& hb, int i) {
  if (hb->_breakPositions.empty()) return -1;

  if (hb->_breakPositions.size() == 1 && hb->_breakPositions[0] <= i)
    return hb->_breakPositions[0];

  size_t pos = 0;
  for (; pos < hb->_breakPositions.size(); pos++) {
    if (hb->_breakPositions[pos] > i) {
      if (pos == 0) return -1;
      return hb->_breakPositions[pos - 1];
    }
  }

  return hb->_breakPositions[pos - 1];
}