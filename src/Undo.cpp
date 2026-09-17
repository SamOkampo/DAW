#include "flowdaw/Undo.hpp"
namespace flowdaw {
void UndoStack::commit(Project before,Project after,std::string name){ if(cursor_<entries_.size()) entries_.erase(entries_.begin()+static_cast<std::ptrdiff_t>(cursor_),entries_.end()); entries_.push_back({std::move(before),std::move(after),std::move(name)}); cursor_=entries_.size(); }
bool UndoStack::undo(Project& p){ if(cursor_==0) return false; --cursor_; p=entries_[cursor_].before; return true; }
bool UndoStack::redo(Project& p){ if(cursor_>=entries_.size()) return false; p=entries_[cursor_].after; ++cursor_; return true; }
std::string UndoStack::undoName() const { return cursor_?entries_[cursor_-1].name:""; }
}
