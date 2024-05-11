// globals addresses
#define ROM_START 128
#define ROM_SIZE 4096  // includes first 128B as well, but inaccessible

// words start address, used in findword
// if changed wbase+1 in base.sf must be updated
#define WORDS_BASE 600

#define IVT_BASE ROM_SIZE
#define IVT_SIZE NUM_INT*IVT_ENTRY_SIZE // both HW and SW interrupts

#define PAD      5120
#define FRAMEBUF 6144
#define HASH_TABLE 8192

// variables
#define TIB_END 9216
#define TIB 9218
#define CRNT_CODE_BASE 9220
#define LAST_WORD_END 9224
#define HERE 9228
#define INLINING 9232

// these only appear in base.sf but are listed here
#define JUMP_COUNT 9234
#define JUMP_VECTOR 9236
#define JUMP_VECTOR_END 9276
#define LABELS_VECTOR JUMP_VECTOR_END
#define LABELS_VECTOR_END 9296

#define TIB_OFFSET 9472
#define TIB_SIZE 256
#define TIB_END_ADDR 9728
// stacks in between
#define HIGH_WORDS 11264

#define DS_OFFSET TIB_END_ADDR
#define RS_OFFSET HIGH_WORDS-4