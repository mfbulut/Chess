#define TITLE "Chess"

#define WINDOW_WIDTH 738
#define WINDOW_HEIGHT 767

#include "smallchesslib.h"
#include "audio.h"

SCL_Game game;
SCL_SquareSet possible_moves;
int last_move_from = -1;
int last_move_to = -1;
int selected_square = -1;

char anim_piece = '.';
float anim_progress = 0.0f;
bool anim_active = false;

char fen_string[SCL_FEN_MAX_LENGTH] = { 0 };

void copy_to_clipboard() {
    SCL_boardToFEN(game.board, &fen_string[0]);
    const size_t len = strlen(fen_string) + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), fen_string, len);
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

vec2 square_to_screen(int square) {
    int x = square % 8;
    int y = square / 8;
    return (vec2){ x * 90.0f, 630 - y * 90.0f };
}

int screen_to_square(vec2 pos) {
    int clicked_x = (int)pos.x / 90;
    int clicked_y = (int)(720 - pos.y) / 90;

    if (clicked_x >= 0 && clicked_x < 8 && clicked_y >= 0 && clicked_y < 8) {
        return clicked_x + clicked_y * 8;
    }

    return -1;
}

void start_piece_animation(char piece) {
    anim_piece = piece;
    anim_progress = 0.0f;
    anim_active = true;
}

void game_init() {
    SCL_gameInit(&game, 0);
    SCL_randomBetterSeed(rand());
    SCL_squareSetClear(possible_moves);
    selected_square = -1;
    last_move_from = -1;
    last_move_to = -1;
    anim_active = false;
}

void ai_move() {
    uint8_t depth = 2;
    uint8_t extraDepth = 3;
    uint8_t endgameDepth = 1;
    uint8_t randomness = game.ply < 2 ? 1 : 0;
    uint8_t rs0, rs1;
    uint8_t s0, s1;
    char promoteTo;

    SCL_gameGetRepetiotionMove(&game, &rs0, &rs1);
    SCL_getAIMove(game.board, depth, extraDepth, endgameDepth, SCL_boardEvaluateStatic, SCL_randomBetter, randomness, rs0, rs1, &s0, &s1, &promoteTo);

    char moving_piece = game.board[s0];

    last_move_from = s0;
    last_move_to = s1;

    start_piece_animation(moving_piece);

    SCL_gameMakeMove(&game, s0, s1, promoteTo);
}

void draw_piece_at_pos(vec2 pos, float piece, float piece_color) {
    draw((vec2){ 90 * piece, 720 + 90 * piece_color }, (vec2){ 90, 90 }, pos, (color){255, 255, 255, 255});
}

void draw_piece(float x, float y, float piece, float piece_color) {
    draw_piece_at_pos((vec2){ x * 90, 630 - y * 90 }, piece, piece_color);
}

float get_piece_type(char piece) {
    switch (piece) {
        case 'K': case 'k': return 0;
        case 'Q': case 'q': return 1;
        case 'B': case 'b': return 2;
        case 'N': case 'n': return 3;
        case 'R': case 'r': return 4;
        case 'P': case 'p': return 5;
        default: return -1;
    }
}

void game_update() {
    if (key_pressed('R')) {
        game_init();
    }

    if (key_pressed('C')) {
        copy_to_clipboard();
    }

    if (key_pressed('Z')) {
        if (SCL_gameUndoMove(&game)) {
            if (SCL_gameUndoMove(&game)) {
                trigger_sfx();
                SCL_squareSetClear(possible_moves);
                selected_square = -1;
                last_move_from = -1;
                last_move_to = -1;
                anim_active = false;
            }
        }
    }

    if (anim_active) {
        anim_progress += 2.0 * delta_time();
        if (anim_progress >= 1.0f) {
            anim_progress = 1.0f;
            anim_active = false;
        }
    }

    if (SCL_boardGameOver(game.board)) {

    } else {
        if (SCL_boardWhitesTurn(game.board)) {
            if (key_pressed('H')) {
                ai_move();
                trigger_sfx();
                selected_square = -1;
                SCL_squareSetClear(possible_moves);
            }

            vec2 mouse_pos = mouse_position();
            int hovered_square = screen_to_square(mouse_pos);

            if (mouse_pressed(0)) {
                if (hovered_square != -1) {
                    char clicked_piece = game.board[hovered_square];
                    if (clicked_piece != '.' && SCL_pieceIsWhite(clicked_piece)) {
                        if (selected_square == hovered_square) {
                            selected_square = -1;
                            SCL_squareSetClear(possible_moves);
                        } else {
                            selected_square = hovered_square;
                            SCL_squareSetClear(possible_moves);
                            SCL_boardGetMoves(game.board, hovered_square, possible_moves);
                            trigger_sfx();
                        }
                    } else if (selected_square != -1 && SCL_squareSetContains(possible_moves, hovered_square)) {
                        char promotion = 0;
                        char moving_piece = game.board[selected_square];
                        int clicked_y = hovered_square / 8;

                        if ((moving_piece == 'p' && clicked_y == 0) ||
                            (moving_piece == 'P' && clicked_y == 7)) {
                            promotion = SCL_pieceIsWhite(moving_piece) ? 'Q' : 'q';
                        }

                        last_move_from = selected_square;
                        last_move_to = hovered_square;
                        start_piece_animation(moving_piece);

                        SCL_gameMakeMove(&game, selected_square, hovered_square, promotion);
                        trigger_sfx();

                        selected_square = -1;
                        SCL_squareSetClear(possible_moves);
                    } else {
                        selected_square = -1;
                        SCL_squareSetClear(possible_moves);
                    }
                }
            }
        } else if(!anim_active) {
            ai_move();
            trigger_sfx();
            selected_square = -1;
            SCL_squareSetClear(possible_moves);
        }
    }

    draw((vec2){ 0, 0 }, (vec2){ 720, 720 }, (vec2){ 0, 0 }, (color){255, 255, 255, 255});

    if (last_move_from != -1) {
        vec2 from_pos = square_to_screen(last_move_from);
        draw((vec2){ 0, 0 }, (vec2){ 89, 89 }, from_pos, (color){0, 110, 140, 128});
    }

    if (last_move_to != -1) {
        vec2 to_pos = square_to_screen(last_move_to);
        draw((vec2){ 0, 0 }, (vec2){ 89, 89 }, to_pos, (color){0, 110, 140, 128});
    }

    if (selected_square != -1) {
        vec2 sel_pos = square_to_screen(selected_square);
        draw((vec2){ 0, 0 }, (vec2){ 89, 89 }, sel_pos, (color){216, 216, 0, 128});
    }

    SCL_SQUARE_SET_ITERATE_BEGIN(possible_moves)
    {
        vec2 move_pos = square_to_screen(iteratedSquare);
        draw((vec2){ 0, 0 }, (vec2){ 89, 89 }, move_pos, (color){32, 196, 32, 128});
    }
    SCL_SQUARE_SET_ITERATE_END

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            char piece = game.board[x + y * 8];
            uint8_t square = x + y * 8;

            if (piece == 'K' && SCL_boardSquareAttacked(game.board, square, 0)) {
                draw((vec2){ 0, 0 }, (vec2){ 89, 89 }, (vec2){ x * 90.0f, 630 - y * 90.0f }, (color){216, 0, 0, 180});
            } else if (piece == 'k' && SCL_boardSquareAttacked(game.board, square, 1)) {
                draw((vec2){ 0, 0 }, (vec2){ 89, 89 }, (vec2){ x * 90.0f, 630 - y * 90.0f }, (color){216, 0, 0, 180});
            }
        }
    }

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int square = x + y * 8;
            char piece = game.board[square];

            if ((anim_active && (square == last_move_from || square == last_move_to))) {
                continue;
            }

            float piece_type = get_piece_type(piece);
            float piece_color = !SCL_pieceIsWhite(piece);

            draw_piece(x, y, piece_type, piece_color);
        }
    }

    if (anim_active) {
        vec2 from_pos = square_to_screen(last_move_from);
        vec2 to_pos = square_to_screen(last_move_to);

        float eased_progress = ease_out_cubic(anim_progress);
        vec2 current_pos = lerp_vec2(from_pos, to_pos, eased_progress);

        float piece_type = get_piece_type(anim_piece);
        float piece_color = !SCL_pieceIsWhite(anim_piece);

        if (piece_type >= 0) {
            draw_piece_at_pos(current_pos, piece_type, piece_color);
        }
    }
}
