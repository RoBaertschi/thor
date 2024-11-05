#include "token.h"

char const *token_type_str(TokenType type) {
    switch (type) {
#define X(upper, lower)      \
    case TOKEN_TYPE_##upper: \
        return #lower;
        TOKENS
#undef X
        default:
            return "invalid token type";
    }
}
