#define TITLE "Chess"

#define WINDOW_WIDTH 738
#define WINDOW_HEIGHT 767

#include "smallchesslib.h"
#include "audio.h"

// Controls
// R - Reset
// Z - Undo
// C - Copy FEN
// H - Get AI Move

SCL_Game game;
SCL_SquareSet possible_moves;
char fen_string[SCL_FEN_MAX_LENGTH] = { 0 };
int selected_square = -1;
int last_move_from = -1;
int last_move_to = -1;

void game_init() {
    SCL_gameInit(&game, 0);
    SCL_randomBetterSeed(rand());
    SCL_squareSetClear(possible_moves);
    selected_square = -1;
    last_move_from = -1;
    last_move_to = -1;
}

void ai_move() {
    uint8_t depth = 3;
    uint8_t extraDepth = 3;
    uint8_t endgameDepth = 2;
    uint8_t randomness = game.ply < 2 ? 1 : 0;
    uint8_t rs0, rs1;
    uint8_t s0, s1;
    char promoteTo;

    SCL_gameGetRepetiotionMove(&game, &rs0, &rs1);
    SCL_getAIMove(game.board, depth, extraDepth, endgameDepth, SCL_boardEvaluateStatic, SCL_randomBetter, randomness, rs0, rs1, &s0, &s1, &promoteTo);

    last_move_from = s0;
    last_move_to = s1;

    SCL_gameMakeMove(&game, s0, s1, promoteTo);
}

void draw_piece(float x, float y, float piece, float color) {
    draw({ 90 * piece, 720 + 90 * color }, { 90, 90 }, { x * 90, 630 - y * 90 }, {255, 255, 255, 255});
}

void game_update() {
    if (SCL_boardGameOver(game.board)) {
        // Todo win and lose inducators
    } else {
        if (SCL_boardWhitesTurn(game.board)) {
            if (key_pressed('H')) {
                ai_move();
                trigger_sound();
                selected_square = -1;
                SCL_squareSetClear(possible_moves);
            }

            if (key_pressed('Z')) {
                if (SCL_gameUndoMove(&game)) {
                    if (SCL_gameUndoMove(&game)) {
                        trigger_sound();
                        selected_square = -1;
                        SCL_squareSetClear(possible_moves);
                        last_move_from = -1;
                        last_move_to = -1;
                    }
                }
            }

            if (key_pressed('R')) {
                game_init();
            }

            if (key_pressed('C')) {
                SCL_boardToFEN(game.board, &fen_string[0]);
                const size_t len = strlen(fen_string) + 1;
                HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
                memcpy(GlobalLock(hMem), fen_string, len);
                GlobalUnlock(hMem);
                OpenClipboard(0);
                EmptyClipboard();
                SetClipboardData(CF_TEXT, hMem);
                CloseClipboard();
            }

            if (mouse_pressed(0)) {
                vec2 pos = mouse_position();
                int clicked_x = (int)pos.x / 90;
                int clicked_y = (int)(720 - pos.y) / 90;

                if (clicked_x >= 0 && clicked_x < 8 && clicked_y >= 0 && clicked_y < 8) {
                    uint8_t clicked_square = clicked_x + clicked_y * 8;

                    if (selected_square != -1) {
                        if (SCL_squareSetContains(possible_moves, clicked_square)) {
                            char promotion = 0;
                            char moving_piece = game.board[selected_square];
                            if ((moving_piece == 'p' && clicked_y == 0) ||
                                (moving_piece == 'P' && clicked_y == 7)) {
                                promotion = SCL_pieceIsWhite(moving_piece) ? 'Q' : 'q';
                            }

                            last_move_from = selected_square;
                            last_move_to = clicked_square;

                            SCL_gameMakeMove(&game, selected_square, clicked_square, promotion);
                            trigger_sound();

                            selected_square = -1;
                            SCL_squareSetClear(possible_moves);
                        } else if (clicked_square == selected_square) {
                            selected_square = -1;
                            SCL_squareSetClear(possible_moves);
                            trigger_sound();
                        } else {
                            char clicked_piece = game.board[clicked_square];
                            if (clicked_piece != '.' && SCL_pieceIsWhite(clicked_piece)) {
                                selected_square = clicked_square;
                                SCL_squareSetClear(possible_moves);
                                SCL_boardGetMoves(game.board, clicked_square, possible_moves);
                                trigger_sound();
                            } else {
                                selected_square = -1;
                                SCL_squareSetClear(possible_moves);
                            }
                        }
                    } else {
                        char clicked_piece = game.board[clicked_square];
                        if (clicked_piece != '.' && SCL_pieceIsWhite(clicked_piece)) {
                            selected_square = clicked_square;
                            SCL_squareSetClear(possible_moves);
                            SCL_boardGetMoves(game.board, clicked_square, possible_moves);
                            trigger_sound();
                        }
                    }
                }
            }
        } else {
            ai_move();
            trigger_sound();
            selected_square = -1;
            SCL_squareSetClear(possible_moves);
        }
    }

    draw({ 0, 0 }, { 720, 720 }, { 0, 0 }, {255, 255, 255, 255});

    if (last_move_from != -1) {
        int from_x = last_move_from % 8;
        int from_y = last_move_from / 8;
        draw({ 0, 0 }, { 89, 89 }, { (float)(from_x * 90), 630 - (float)(from_y * 90) }, {90, 80, 10, 128});
    }

    if (last_move_to != -1) {
        int to_x = last_move_to % 8;
        int to_y = last_move_to / 8;
        draw({ 0, 0 }, { 89, 89 }, { (float)(to_x * 90), 630 - (float)(to_y * 90) }, {255, 200, 100, 255});
    }

    if (selected_square != -1) {
        int sel_x = selected_square % 8;
        int sel_y = selected_square / 8;
        draw({ 0, 0 }, { 89, 89 }, { (float)(sel_x * 90), 630 - (float)(sel_y * 90) }, {216, 216, 0, 255});
    }

    SCL_SQUARE_SET_ITERATE_BEGIN(possible_moves)
    {
        int move_x = iteratedSquare % 8;
        int move_y = iteratedSquare / 8;
        draw({ 0, 0 }, { 89, 89 }, { (float)(move_x * 90), 630 - (float)(move_y * 90) }, {32, 196, 32, 140});
    }
    SCL_SQUARE_SET_ITERATE_END

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            char piece = game.board[x + y * 8];
            uint8_t square = x + y * 8;

            if (piece == 'K' && SCL_boardSquareAttacked(game.board, square, 0)) {
                draw({ 0, 0 }, { 89, 89 }, { (float)(x * 90), 630 - (float)(y * 90) }, {216, 0, 0, 180});
            } else if (piece == 'k' && SCL_boardSquareAttacked(game.board, square, 1)) {
                draw({ 0, 0 }, { 89, 89 }, { (float)(x * 90), 630 - (float)(y * 90) }, {216, 0, 0, 180});
            }
        }
    }

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            char piece = game.board[x + y * 8];
            switch (piece) {
                case 'K': draw_piece(x, y, 0, 0); break;
                case 'Q': draw_piece(x, y, 1, 0); break;
                case 'B': draw_piece(x, y, 2, 0); break;
                case 'N': draw_piece(x, y, 3, 0); break;
                case 'R': draw_piece(x, y, 4, 0); break;
                case 'P': draw_piece(x, y, 5, 0); break;
                case 'k': draw_piece(x, y, 0, 1); break;
                case 'q': draw_piece(x, y, 1, 1); break;
                case 'b': draw_piece(x, y, 2, 1); break;
                case 'n': draw_piece(x, y, 3, 1); break;
                case 'r': draw_piece(x, y, 4, 1); break;
                case 'p': draw_piece(x, y, 5, 1); break;
            }
        }
    }
}