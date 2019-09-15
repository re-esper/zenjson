namespace zjson {

#define ZJSON_BLOCK_SIZE    8192
class Allocator {
public:
    Allocator() : blocksHead(nullptr), freeBlocksHead(nullptr) {};
    Allocator(const Allocator &) = delete;
    Allocator &operator=(const Allocator &) = delete;
    ~Allocator() {
        deallocate();
    }
    inline void *allocate(size_t size) {
        size = (size + 7) & ~7;
        if (blocksHead && blocksHead->used + size <= ZJSON_BLOCK_SIZE) {
            char *p = (char *)blocksHead + blocksHead->used;
            blocksHead->used += size;
            return p;
        }
        size_t allocSize = sizeof(Block) + size;
        Block *block = freeBlocksHead;
        if (block && block->size >= allocSize) { // reuse free block
            freeBlocksHead = block->next;
        }
        else { // allocate new block
            block = (Block *)malloc(allocSize <= ZJSON_BLOCK_SIZE ? ZJSON_BLOCK_SIZE : allocSize);
            if (!block) return nullptr;
            block->size = allocSize;
        }
        block->used = allocSize;
        if (allocSize <= ZJSON_BLOCK_SIZE || !blocksHead) { // push_front
            block->next = blocksHead;
            blocksHead = block;
        }
        else { // insert
            block->next = blocksHead->next;
            blocksHead->next = block;
        }
        return (char *)block + sizeof(Block);
    }
    inline void reset() {
        if (blocksHead) {
            Block* block = blocksHead;
            while (block && block->next) block = block->next;
            block->next = freeBlocksHead;
            freeBlocksHead = blocksHead;
            blocksHead = nullptr;
        }
    }
    void deallocate() {
        freeBlockChain(blocksHead);
        blocksHead = nullptr;
        freeBlockChain(freeBlocksHead);
        freeBlocksHead = nullptr;
    }
private:
    struct Block {
        Block *next;
        size_t used;
        size_t size;
    } *blocksHead, *freeBlocksHead;
    inline void freeBlockChain(Block* block) {
        while (block) {
            Block* nextblock = block->next;
            free(block);
            block = nextblock;
        }
    }
};

} // namespace zjson
