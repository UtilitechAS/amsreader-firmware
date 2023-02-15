#include "LlcParser.h"

int8_t LLCParser::parse(uint8_t *buf, DataParserContext &ctx) {
    LLCHeader* llc = (LLCHeader*) buf;
    ctx.length -= 3;
    return 3;
}