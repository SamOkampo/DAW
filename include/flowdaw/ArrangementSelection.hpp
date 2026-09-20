#pragma once

#include "flowdaw/Types.hpp"

namespace flowdaw {

// UI/control-thread-only Arrangement selection state. Selection is keyed by
// persistent block IDs instead of vector indices so unrelated edits/reordering
// cannot silently retarget the selection.
class ArrangementSelection {
public:
    enum class Kind { None, AudioClip, PatternClip };

    struct Item {
        Kind kind = Kind::None;
        Id id = 0;
        Id trackId = 0;

        [[nodiscard]] bool valid() const noexcept {
            return kind != Kind::None && id != 0 && trackId != 0;
        }

        friend bool operator==(const Item&, const Item&) = default;
    };

    [[nodiscard]] const Item& item() const noexcept { return item_; }
    [[nodiscard]] bool empty() const noexcept { return !item_.valid(); }
    [[nodiscard]] bool contains(Kind kind, Id trackId, Id id) const noexcept {
        return item_ == Item{kind, id, trackId};
    }

    void selectAudio(Id trackId, Id clipId) noexcept {
        item_ = {Kind::AudioClip, clipId, trackId};
        normalize();
    }

    void selectPattern(Id trackId, Id placementId) noexcept {
        item_ = {Kind::PatternClip, placementId, trackId};
        normalize();
    }

    void clear() noexcept { item_ = {}; }

private:
    void normalize() noexcept {
        if (!item_.valid())
            clear();
    }

    Item item_;
};

} // namespace flowdaw
