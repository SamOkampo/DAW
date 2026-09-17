#pragma once
#include "flowdaw/Project.hpp"
#include <functional>
#include <string>
#include <vector>
namespace flowdaw {
class UndoStack {
public:
    void commit(Project before,Project after,std::string name);
    bool undo(Project& p);
    bool redo(Project& p);
    std::string undoName() const;
private:
    struct Entry { Project before,after; std::string name; };
    std::vector<Entry> entries_; std::size_t cursor_=0;
};
}
