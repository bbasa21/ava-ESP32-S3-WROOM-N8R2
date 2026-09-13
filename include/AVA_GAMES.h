#ifndef AVA_GAMES_H
#define AVA_GAMES_H

#include <Arduino.h>

// ==================================================
// AVA GAMES ENGINE
//
// مسئولیت:
// - مدیریت بازی فعلی
// - GAME_LOAD
// - GAME_START
// - GAME_ANSWER
// - GAME_END
// - منطق MATH BATTLE
// - منطق LOGIC BATTLE
// - مدیریت 4 گزینه‌ی مشترک برای هر دو بازیکن
//
// BLE فقط پیام را تحویل این لایه می‌دهد.
// این فایل منطق اصلی بازی را مدیریت می‌کند.
//
// PLAYER:
//   0 = ALI
//   1 = AVA
//
// ترتیب بازی:
//   ALI -> AVA -> SCORE -> RESULT -> NEXT ROUND
//
// قانون امتیازدهی:
//   ALI درست  -> ALI +1
//   AVA درست  -> AVA +1
//   هر دو درست -> هر دو +1
//   هر دو غلط  -> هیچ‌کس امتیاز نمی‌گیرد
// ==================================================

namespace AvaGames {

// ==================================================
// GAME IDs
// ==================================================

static constexpr const char* GAME_NONE =
"NONE";

static constexpr const char* GAME_MATH_BATTLE =
"MATH_BATTLE";

static constexpr const char* GAME_LOGIC_BATTLE =
"LOGIC_BATTLE";

// ==================================================
// PLAYER IDs
// ==================================================

static constexpr int PLAYER_ALI = 0;
static constexpr int PLAYER_AVA = 1;

// ==================================================
// GAME STATE
// ==================================================

enum GameState
{
GAME_IDLE,
GAME_LOADED,
GAME_RUNNING,
GAME_FINISHED
};

inline GameState& state()
{
static GameState value = GAME_IDLE;
return value;
}

// ==================================================
// CURRENT GAME
// ==================================================

inline String& currentGame()
{
static String value = GAME_NONE;
return value;
}

// ==================================================
// SHARED SCORE
//
// هر دو بازی از همین سیستم امتیاز استفاده می‌کنند.
// ==================================================

inline int& aliScore()
{
static int value = 0;
return value;
}

inline int& avaScore()
{
static int value = 0;
return value;
}

inline int& currentRound()
{
static int value = 0;
return value;
}

inline int& totalRounds()
{
static int value = 5;
return value;
}

// ==================================================
// MATH QUESTION STATE
// ==================================================

inline int& questionA()
{
static int value = 0;
return value;
}

inline int& questionB()
{
static int value = 0;
return value;
}

inline char& questionOperator()
{
static char value = '+';
return value;
}

inline int& correctAnswer()
{
static int value = 0;
return value;
}

// ==================================================
// LOGIC QUESTION STATE
// ==================================================

enum LogicQuestionType
{
LOGIC_SEQUENCE,
LOGIC_PATTERN,
LOGIC_ODD_ONE_OUT,
LOGIC_DEDUCTION
};

inline LogicQuestionType& logicQuestionType()
{
static LogicQuestionType value =
LOGIC_SEQUENCE;


return value;


}

inline int& logicQuestionIndex()
{
static int value = -1;
return value;
}

inline String& logicQuestionText()
{
static String value = "";
return value;
}

// ==================================================
// MULTIPLE CHOICE OPTIONS
//
// همان 4 گزینه برای ALI و AVA.
//
// جواب درست فقط داخل Game Engine نگهداری می‌شود.
// ==================================================

inline int* options()
{
static int value[4] =
{
0,
0,
0,
0
};


return value;


}

inline int& correctOptionIndex()
{
static int value = 0;
return value;
}

// ==================================================
// ROUND SCORE APPLICATION GUARD
// ==================================================

inline bool& scoreApplied()
{
static bool value = false;
return value;
}

// ==================================================
// ROUND ANSWERS
//
// -1 means player has not answered yet.
// ==================================================

inline int& aliAnswer()
{
static int value = -1;
return value;
}

inline int& avaAnswer()
{
static int value = -1;
return value;
}

inline bool& aliAnswered()
{
static bool value = false;
return value;
}

inline bool& avaAnswered()
{
static bool value = false;
return value;
}

inline bool& aliCorrect()
{
static bool value = false;
return value;
}

inline bool& avaCorrect()
{
static bool value = false;
return value;
}

// ==================================================
// ROUND RESULT
// ==================================================

enum RoundResult
{
ROUND_PENDING,
ROUND_ALI_WIN,
ROUND_AVA_WIN,
ROUND_DRAW,
ROUND_NOBODY
};

inline RoundResult& roundResult()
{
static RoundResult value = ROUND_PENDING;
return value;
}

// ==================================================
// AVA DIFFICULTY
//
// EASY   -> ~10% wrong
// MEDIUM -> ~20% wrong
// HARD   -> ~35% wrong
// ==================================================

enum AvaDifficulty
{
AVA_EASY,
AVA_MEDIUM,
AVA_HARD
};

inline AvaDifficulty& avaDifficulty()
{
static AvaDifficulty value = AVA_MEDIUM;
return value;
}

// ==================================================
// RESET ROUND
// ==================================================

inline void resetRoundState()
{
aliAnswer() = -1;
avaAnswer() = -1;


aliAnswered() = false;
avaAnswered() = false;

aliCorrect() = false;
avaCorrect() = false;

roundResult() = ROUND_PENDING;

scoreApplied() = false;


}

// ==================================================
// RESET GAME
// ==================================================

inline void resetGameState()
{
aliScore() = 0;
avaScore() = 0;


currentRound() = 0;

questionA() = 0;
questionB() = 0;

questionOperator() = '+';
correctAnswer() = 0;

logicQuestionIndex() = -1;
logicQuestionType() = LOGIC_SEQUENCE;
logicQuestionText() = "";

options()[0] = 0;
options()[1] = 0;
options()[2] = 0;
options()[3] = 0;

correctOptionIndex() = 0;

resetRoundState();


}

// ==================================================
// CHECK OPTION DUPLICATE
// ==================================================

inline bool optionExists(
int value,
int count
)
{
for (int i = 0; i < count; i++)
{
if (options()[i] == value)
{
return true;
}
}


return false;


}

// ==================================================
// GENERATE 4 MULTIPLE-CHOICE OPTIONS
//
// یک جواب صحیح
// سه جواب غلط
// ==================================================

inline void generateOptions()
{
options()[0] = correctAnswer();


int generated = 1;
int attempts = 0;

while (generated < 4 &&
       attempts < 100)
{
    attempts++;

    int offset =
        random(1, 6);

    bool add =
        random(0, 2) == 0;

    int wrongAnswer;

    if (add)
    {
        wrongAnswer =
            correctAnswer() + offset;
    }
    else
    {
        wrongAnswer =
            correctAnswer() - offset;
    }

    if (wrongAnswer < 0)
    {
        wrongAnswer = 0;
    }

    if (!optionExists(
            wrongAnswer,
            generated))
    {
        options()[generated] =
            wrongAnswer;

        generated++;
    }
}

int fallbackOffset = 1;

while (generated < 4)
{
    int wrongAnswer =
        correctAnswer() +
        fallbackOffset;

    fallbackOffset++;

    if (!optionExists(
            wrongAnswer,
            generated))
    {
        options()[generated] =
            wrongAnswer;

        generated++;
    }
}

// --------------------------------------------------
// Shuffle
// --------------------------------------------------

for (int i = 3; i > 0; i--)
{
    int j =
        random(0, i + 1);

    int temp =
        options()[i];

    options()[i] =
        options()[j];

    options()[j] =
        temp;
}

// --------------------------------------------------
// Find correct option index.
// --------------------------------------------------

correctOptionIndex() = 0;

for (int i = 0; i < 4; i++)
{
    if (options()[i] ==
        correctAnswer())
    {
        correctOptionIndex() =
            i;

        break;
    }
}


}

// ==================================================
// RANDOM MATH QUESTION
//
// این بخش همان منطق MATH BATTLE فعلی است.
// ==================================================

inline void generateMathQuestion()
{
int operation =
random(0, 3);


// --------------------------------------------------
// ADDITION
// --------------------------------------------------

if (operation == 0)
{
    int a = random(1, 21);
    int b = random(1, 21);

    questionA() = a;
    questionB() = b;

    questionOperator() = '+';

    correctAnswer() =
        a + b;
}

// --------------------------------------------------
// SUBTRACTION
// --------------------------------------------------

else if (operation == 1)
{
    int a = random(1, 21);
    int b = random(1, 21);

    if (b > a)
    {
        int temp = a;
        a = b;
        b = temp;
    }

    questionA() = a;
    questionB() = b;

    questionOperator() = '-';

    correctAnswer() =
        a - b;
}

// --------------------------------------------------
// MULTIPLICATION
// --------------------------------------------------

else
{
    int a = random(1, 11);
    int b = random(1, 11);

    questionA() = a;
    questionB() = b;

    questionOperator() = '*';

    correctAnswer() =
        a * b;
}

generateOptions();

resetRoundState();

Serial.print("[GAME] QUESTION: ");

Serial.print(questionA());

Serial.print(" ");

Serial.print(questionOperator());

Serial.print(" ");

Serial.print(questionB());

Serial.print(" = ");

Serial.println(correctAnswer());

Serial.print("[GAME] OPTIONS: ");

Serial.print(options()[0]);

Serial.print(", ");

Serial.print(options()[1]);

Serial.print(", ");

Serial.print(options()[2]);

Serial.print(", ");

Serial.println(options()[3]);

Serial.println(
    "[GAME] Waiting for ALI answer..."
);


}

// ==================================================
// LOGIC QUESTION BANK
//
// فعلاً سؤال‌های Logic به صورت عددی هستند تا
// با سیستم 4 گزینه‌ای فعلی هماهنگ باشند.
//
// بعداً می‌توانیم Question Engine را توسعه دهیم
// تا سؤال‌های متنی و چندنوعی هم داشته باشد.
// ==================================================

struct LogicQuestion
{
const char* question;
int options[4];
int correctIndex;
LogicQuestionType type;
AvaDifficulty difficulty;
};

// --------------------------------------------------
// LOGIC QUESTIONS
// --------------------------------------------------

static const LogicQuestion LOGIC_QUESTIONS[] =
{
// ==================================================
// EASY - SEQUENCE
// ==================================================


{
    "2, 4, 8, 16, ?",
    {20, 24, 32, 34},
    2,
    LOGIC_SEQUENCE,
    AVA_EASY
},

{
    "3, 6, 12, 24, ?",
    {36, 42, 48, 54},
    2,
    LOGIC_SEQUENCE,
    AVA_EASY
},

{
    "5, 10, 20, 40, ?",
    {60, 70, 80, 90},
    2,
    LOGIC_SEQUENCE,
    AVA_EASY
},

{
    "1, 2, 4, 8, ?",
    {10, 12, 16, 18},
    2,
    LOGIC_SEQUENCE,
    AVA_EASY
},

// ==================================================
// MEDIUM - SEQUENCE
// ==================================================

{
    "1, 4, 9, 16, ?",
    {20, 24, 25, 36},
    2,
    LOGIC_SEQUENCE,
    AVA_MEDIUM
},

{
    "2, 6, 12, 20, ?",
    {28, 30, 32, 36},
    1,
    LOGIC_SEQUENCE,
    AVA_MEDIUM
},

{
    "3, 5, 9, 17, ?",
    {25, 29, 33, 35},
    2,
    LOGIC_SEQUENCE,
    AVA_MEDIUM
},

{
    "10, 13, 19, 28, ?",
    {37, 38, 40, 42},
    2,
    LOGIC_SEQUENCE,
    AVA_MEDIUM
},

// ==================================================
// HARD - SEQUENCE
// ==================================================

{
    "2, 3, 5, 9, 17, ?",
    {25, 29, 31, 33},
    3,
    LOGIC_SEQUENCE,
    AVA_HARD
},

{
    "1, 2, 6, 24, ?",
    {96, 100, 120, 144},
    2,
    LOGIC_SEQUENCE,
    AVA_HARD
},

{
    "4, 7, 13, 25, ?",
    {37, 43, 49, 51},
    2,
    LOGIC_SEQUENCE,
    AVA_HARD
},

// ==================================================
// PATTERN
// ==================================================

{
    "2, 5, 11, 23, ?",
    {35, 41, 47, 48},
    2,
    LOGIC_PATTERN,
    AVA_MEDIUM
},

{
    "1, 3, 7, 15, ?",
    {23, 27, 31, 35},
    2,
    LOGIC_PATTERN,
    AVA_MEDIUM
},

{
    "3, 8, 18, 38, ?",
    {68, 72, 78, 80},
    2,
    LOGIC_PATTERN,
    AVA_HARD
},

{
    "2, 4, 10, 22, ?",
    {40, 42, 46, 48},
    2,
    LOGIC_PATTERN,
    AVA_HARD
}


};

// ==================================================
// LOGIC QUESTION COUNT
// ==================================================

inline int getLogicQuestionCount()
{
return
sizeof(LOGIC_QUESTIONS) /
sizeof(LOGIC_QUESTIONS[0]);
}

// ==================================================
// GENERATE LOGIC QUESTION
//
// سؤال بر اساس difficulty انتخاب می‌شود.
//
// اگر difficulty خاصی پیدا نشود، از هر سؤال
// موجود انتخاب می‌کنیم تا Engine هیچ‌وقت گیر نکند.
// ==================================================

inline void generateLogicQuestion()
{
const int count =
getLogicQuestionCount();


if (count <= 0)
{
    Serial.println(
        "[LOGIC] ERROR: Question bank empty."
    );

    return;
}

int candidates[32];
int candidateCount = 0;

// --------------------------------------------------
// Find questions matching AVA difficulty.
// --------------------------------------------------

for (int i = 0;
     i < count && candidateCount < 32;
     i++)
{
    if (LOGIC_QUESTIONS[i].difficulty ==
        avaDifficulty())
    {
        candidates[candidateCount] =
            i;

        candidateCount++;
    }
}

// --------------------------------------------------
// If no matching difficulty exists,
// use complete question bank.
// --------------------------------------------------

if (candidateCount == 0)
{
    for (int i = 0;
         i < count && candidateCount < 32;
         i++)
    {
        candidates[candidateCount] =
            i;

        candidateCount++;
    }
}

int selected =
    random(0, candidateCount);

logicQuestionIndex() =
    candidates[selected];

const LogicQuestion& question =
    LOGIC_QUESTIONS[
        logicQuestionIndex()
    ];

logicQuestionType() =
    question.type;

logicQuestionText() =
    question.question;

// --------------------------------------------------
// Copy options.
// --------------------------------------------------

for (int i = 0; i < 4; i++)
{
    options()[i] =
        question.options[i];
}

// --------------------------------------------------
// Correct option.
// --------------------------------------------------

correctOptionIndex() =
    question.correctIndex;

correctAnswer() =
    options()[
        correctOptionIndex()
    ];

// --------------------------------------------------
// Reset answers.
// --------------------------------------------------

resetRoundState();

// --------------------------------------------------
// Debug.
// --------------------------------------------------

Serial.print(
    "[LOGIC] QUESTION: "
);

Serial.println(
    logicQuestionText()
);

Serial.print(
    "[LOGIC] OPTIONS: "
);

Serial.print(options()[0]);

Serial.print(", ");

Serial.print(options()[1]);

Serial.print(", ");

Serial.print(options()[2]);

Serial.print(", ");

Serial.println(options()[3]);

Serial.print(
    "[LOGIC] CORRECT INDEX: "
);

Serial.println(
    correctOptionIndex()
);

Serial.println(
    "[LOGIC] Waiting for ALI answer..."
);


}

// ==================================================
// GET QUESTION TEXT
//
// برای MATH:
//     12+7
//
// برای LOGIC:
//     متن سؤال
// ==================================================

inline String getQuestionText()
{
if (currentGame() ==
GAME_LOGIC_BATTLE)
{
return logicQuestionText();
}


String result = "";

result += String(questionA());
result += String(questionOperator());
result += String(questionB());

return result;


}

// ==================================================
// AVA CALCULATES CORRECT ANSWER
// ==================================================

inline int calculateCorrectAnswer()
{
return correctAnswer();
}

// ==================================================
// AVA AI ERROR DECISION
// ==================================================

inline bool avaShouldMakeMistake()
{
int roll =
random(0, 100);


switch (avaDifficulty())
{
    case AVA_EASY:
        return roll < 10;

    case AVA_MEDIUM:
        return roll < 20;

    case AVA_HARD:
        return roll < 35;
}

return false;


}

// ==================================================
// GENERATE AVA ANSWER
//
// AVA یکی از 4 گزینه را انتخاب می‌کند.
//
// اگر درست:
//     correct option
//
// اگر اشتباه:
//     یکی از 3 گزینه‌ی غلط
// ==================================================

inline int generateAvaAnswer()
{
if (!avaShouldMakeMistake())
{
Serial.println(
"[GAME] AVA decided to answer CORRECTLY."
);


    return correctAnswer();
}

int wrongIndexes[3];
int count = 0;

for (int i = 0; i < 4; i++)
{
    if (i != correctOptionIndex())
    {
        wrongIndexes[count] =
            i;

        count++;
    }
}

if (count <= 0)
{
    return correctAnswer();
}

int selected =
    random(0, count);

int answer =
    options()[
        wrongIndexes[selected]
    ];

Serial.println(
    "[GAME] AVA decided to make a mistake."
);

return answer;


}

// ==================================================
// EVALUATE ROUND
// ==================================================

inline RoundResult evaluateRound()
{
if (!aliAnswered() ||
!avaAnswered())
{
roundResult() =
ROUND_PENDING;


    return ROUND_PENDING;
}

if (aliCorrect() &&
    !avaCorrect())
{
    roundResult() =
        ROUND_ALI_WIN;

    return ROUND_ALI_WIN;
}

if (!aliCorrect() &&
    avaCorrect())
{
    roundResult() =
        ROUND_AVA_WIN;

    return ROUND_AVA_WIN;
}

if (aliCorrect() &&
    avaCorrect())
{
    roundResult() =
        ROUND_DRAW;

    return ROUND_DRAW;
}

roundResult() =
    ROUND_NOBODY;

return ROUND_NOBODY;


}

// ==================================================
// CHECK BOTH ANSWERS
// ==================================================

inline bool bothPlayersAnswered()
{
return
aliAnswered() &&
avaAnswered();
}

// ==================================================
// APPLY ROUND SCORE
// ==================================================

inline void applyRoundScore()
{
if (scoreApplied())
{
Serial.println(
"[GAME] Score already applied."
);


    return;
}

if (!bothPlayersAnswered())
{
    Serial.println(
        "[GAME] Cannot score: waiting for both answers."
    );

    return;
}

RoundResult result =
    evaluateRound();

// --------------------------------------------------
// ALI
// --------------------------------------------------

if (aliCorrect())
{
    aliScore()++;

    Serial.println(
        "[GAME] ALI +1"
    );
}
else
{
    Serial.println(
        "[GAME] ALI +0"
    );
}

// --------------------------------------------------
// AVA
// --------------------------------------------------

if (avaCorrect())
{
    avaScore()++;

    Serial.println(
        "[GAME] AVA +1"
    );
}
else
{
    Serial.println(
        "[GAME] AVA +0"
    );
}

// --------------------------------------------------
// Result
// --------------------------------------------------

switch (result)
{
    case ROUND_ALI_WIN:

        Serial.println(
            "[GAME] ROUND RESULT: ALI"
        );

        break;

    case ROUND_AVA_WIN:

        Serial.println(
            "[GAME] ROUND RESULT: AVA"
        );

        break;

    case ROUND_DRAW:

        Serial.println(
            "[GAME] ROUND RESULT: BOTH CORRECT"
        );

        break;

    case ROUND_NOBODY:

        Serial.println(
            "[GAME] ROUND RESULT: BOTH WRONG"
        );

        break;

    case ROUND_PENDING:

        Serial.println(
            "[GAME] ROUND RESULT: PENDING"
        );

        return;
}

scoreApplied() = true;

Serial.print(
    "[GAME] SCORE AFTER ROUND: ALI="
);

Serial.print(
    aliScore()
);

Serial.print(
    " AVA="
);

Serial.println(
    avaScore()
);


}

// ==================================================
// GAME LOAD
// ==================================================

inline bool loadGame(
const String& gameId
)
{
String id =
gameId;


id.trim();
id.toUpperCase();

Serial.print("[GAME] Loading: ");
Serial.println(id);

// --------------------------------------------------
// MATH BATTLE
// --------------------------------------------------

if (id == GAME_MATH_BATTLE)
{
    currentGame() =
        GAME_MATH_BATTLE;

    state() =
        GAME_LOADED;

    resetGameState();

    Serial.println(
        "[GAME] MATH BATTLE loaded."
    );

    Serial.print(
        "[GAME] ROUNDS: "
    );

    Serial.println(
        totalRounds()
    );

    return true;
}

// --------------------------------------------------
// LOGIC BATTLE
// --------------------------------------------------

if (id == GAME_LOGIC_BATTLE)
{
    currentGame() =
        GAME_LOGIC_BATTLE;

    state() =
        GAME_LOADED;

    resetGameState();

    Serial.println(
        "[GAME] LOGIC BATTLE loaded."
    );

    Serial.print(
        "[GAME] ROUNDS: "
    );

    Serial.println(
        totalRounds()
    );

    return true;
}

Serial.print(
    "[GAME] Unknown game: "
);

Serial.println(id);

return false;


}

// ==================================================
// GAME START
// ==================================================

inline bool startGame()
{
if (currentGame() == GAME_NONE)
{
Serial.println(
"[GAME] No game loaded."
);


    return false;
}

// --------------------------------------------------
// MATH BATTLE
// --------------------------------------------------

if (currentGame() ==
    GAME_MATH_BATTLE)
{
    resetGameState();

    currentRound() = 1;

    state() =
        GAME_RUNNING;

    Serial.println(
        "[GAME] MATH BATTLE started."
    );

    Serial.print(
        "[GAME] ROUND: "
    );

    Serial.println(
        currentRound()
    );

    generateMathQuestion();

    Serial.println(
        "[GAME] Waiting for ALI to answer first."
    );

    return true;
}

// --------------------------------------------------
// LOGIC BATTLE
// --------------------------------------------------

if (currentGame() ==
    GAME_LOGIC_BATTLE)
{
    resetGameState();

    currentRound() = 1;

    state() =
        GAME_RUNNING;

    Serial.println(
        "[GAME] LOGIC BATTLE started."
    );

    Serial.print(
        "[GAME] ROUND: "
    );

    Serial.println(
        currentRound()
    );

    generateLogicQuestion();

    Serial.println(
        "[LOGIC] Waiting for ALI to answer first."
    );

    return true;
}

return false;


}

// ==================================================
// SUBMIT PLAYER ANSWER
// ==================================================

inline bool submitAnswer(
int player,
int answer
)
{
if (state() != GAME_RUNNING)
{
Serial.println(
"[GAME] Answer rejected: game not running."
);


    return false;
}

if (currentGame() != GAME_MATH_BATTLE &&
    currentGame() != GAME_LOGIC_BATTLE)
{
    Serial.println(
        "[GAME] Answer rejected: wrong game."
    );

    return false;
}

bool correct =
    (answer == correctAnswer());

// ==================================================
// ALI
// ==================================================

if (player == PLAYER_ALI)
{
    if (aliAnswered())
    {
        Serial.println(
            "[GAME] ALI already answered."
        );

        return aliCorrect();
    }

    aliAnswer() =
        answer;

    aliAnswered() =
        true;

    aliCorrect() =
        correct;

    Serial.print(
        "[GAME] ALI ANSWER: "
    );

    Serial.println(
        answer
    );

    if (correct)
    {
        Serial.println(
            "[GAME] ALI CORRECT."
        );
    }
    else
    {
        Serial.println(
            "[GAME] ALI WRONG."
        );
    }

    Serial.println(
        "[GAME] Now waiting for AVA answer."
    );
}

// ==================================================
// AVA
// ==================================================

else if (player == PLAYER_AVA)
{
    if (avaAnswered())
    {
        Serial.println(
            "[GAME] AVA already answered."
        );

        return avaCorrect();
    }

    avaAnswer() =
        answer;

    avaAnswered() =
        true;

    avaCorrect() =
        correct;

    Serial.print(
        "[GAME] AVA ANSWER: "
    );

    Serial.println(
        answer
    );

    if (correct)
    {
        Serial.println(
            "[GAME] AVA CORRECT."
        );
    }
    else
    {
        Serial.println(
            "[GAME] AVA WRONG."
        );
    }
}

else
{
    Serial.println(
        "[GAME] Unknown player."
    );

    return false;
}

// --------------------------------------------------
// Evaluate when both answered.
// --------------------------------------------------

if (bothPlayersAnswered())
{
    RoundResult result =
        evaluateRound();

    switch (result)
    {
        case ROUND_ALI_WIN:

            Serial.println(
                "[GAME] ROUND RESULT: ALI"
            );

            break;

        case ROUND_AVA_WIN:

            Serial.println(
                "[GAME] ROUND RESULT: AVA"
            );

            break;

        case ROUND_DRAW:

            Serial.println(
                "[GAME] ROUND RESULT: BOTH CORRECT"
            );

            break;

        case ROUND_NOBODY:

            Serial.println(
                "[GAME] ROUND RESULT: BOTH WRONG"
            );

            break;

        default:

            break;
    }
}

return correct;


}

// ==================================================
// NEXT ROUND
// ==================================================

inline bool nextRound()
{
if (state() != GAME_RUNNING)
{
return false;
}


if (!bothPlayersAnswered())
{
    Serial.println(
        "[GAME] Cannot advance: "
        "waiting for both answers."
    );

    return false;
}

// --------------------------------------------------
// Score current round.
// --------------------------------------------------

applyRoundScore();

Serial.print(
    "[GAME] SCORE AFTER ROUND "
);

Serial.print(
    currentRound()
);

Serial.print(
    ": ALI="
);

Serial.print(
    aliScore()
);

Serial.print(
    " AVA="
);

Serial.println(
    avaScore()
);

// --------------------------------------------------
// Check game end.
// --------------------------------------------------

if (currentRound() >=
    totalRounds())
{
    state() =
        GAME_FINISHED;

    Serial.print(
        "[GAME] "
    );

    Serial.print(
        currentGame()
    );

    Serial.println(
        " finished."
    );

    Serial.print(
        "[GAME] FINAL ALI SCORE: "
    );

    Serial.println(
        aliScore()
    );

    Serial.print(
        "[GAME] FINAL AVA SCORE: "
    );

    Serial.println(
        avaScore()
    );

    return false;
}

// --------------------------------------------------
// Next round.
// --------------------------------------------------

currentRound()++;

if (currentGame() ==
    GAME_MATH_BATTLE)
{
    generateMathQuestion();
}
else if (currentGame() ==
         GAME_LOGIC_BATTLE)
{
    generateLogicQuestion();
}

Serial.print(
    "[GAME] NEW ROUND: "
);

Serial.println(
    currentRound()
);

Serial.println(
    "[GAME] Waiting for ALI answer..."
);

return true;


}

// ==================================================
// END GAME
// ==================================================

inline void endGame()
{
state() =
GAME_FINISHED;


Serial.println(
    "[GAME] Game ended."
);

Serial.print(
    "[GAME] ALI SCORE: "
);

Serial.println(
    aliScore()
);

Serial.print(
    "[GAME] AVA SCORE: "
);

Serial.println(
    avaScore()
);


}

// ==================================================
// CLEAR / RESET GAME
// ==================================================

inline void reset()
{
currentGame() =
GAME_NONE;


state() =
    GAME_IDLE;

resetGameState();

Serial.println(
    "[GAME] Engine reset."
);


}

// ==================================================
// GAME WINNER
// ==================================================

inline String getWinner()
{
if (aliScore() >
avaScore())
{
return "ALI";
}


if (avaScore() >
    aliScore())
{
    return "AVA";
}

if (state() ==
    GAME_FINISHED)
{
    return "DRAW";
}

return "NONE";


}

// ==================================================
// GETTERS
// ==================================================

inline bool isRunning()
{
return state() ==
GAME_RUNNING;
}

inline bool isLoaded()
{
return state() ==
GAME_LOADED;
}

inline bool isFinished()
{
return state() ==
GAME_FINISHED;
}

inline bool isIdle()
{
return state() ==
GAME_IDLE;
}

// --------------------------------------------------
// Current game
// --------------------------------------------------

inline const String& getCurrentGame()
{
return currentGame();
}

// --------------------------------------------------
// Scores
// --------------------------------------------------

inline int getAliScore()
{
return aliScore();
}

inline int getAvaScore()
{
return avaScore();
}

// --------------------------------------------------
// Round
// --------------------------------------------------

inline int getCurrentRound()
{
return currentRound();
}

inline int getTotalRounds()
{
return totalRounds();
}

// --------------------------------------------------
// Question
// --------------------------------------------------

inline int getQuestionA()
{
return questionA();
}

inline int getQuestionB()
{
return questionB();
}

inline char getQuestionOperator()
{
return questionOperator();
}

inline int getCorrectAnswer()
{
return correctAnswer();
}

inline String getQuestion()
{
return getQuestionText();
}

// --------------------------------------------------
// Logic Question
// --------------------------------------------------

inline LogicQuestionType getLogicQuestionType()
{
return logicQuestionType();
}

inline int getLogicQuestionIndex()
{
return logicQuestionIndex();
}

inline String getLogicQuestionText()
{
return logicQuestionText();
}

// --------------------------------------------------
// Multiple choice
// --------------------------------------------------

inline int getOption(
int index
)
{
if (index < 0 ||
index >= 4)
{
return 0;
}


return options()[index];


}

inline int getCorrectOptionIndex()
{
return correctOptionIndex();
}

// --------------------------------------------------
// Player answers
// --------------------------------------------------

inline int getAliAnswer()
{
return aliAnswer();
}

inline int getAvaAnswer()
{
return avaAnswer();
}

inline bool hasAliAnswered()
{
return aliAnswered();
}

inline bool hasAvaAnswered()
{
return avaAnswered();
}

inline bool isAliCorrect()
{
return aliCorrect();
}

inline bool isAvaCorrect()
{
return avaCorrect();
}

// --------------------------------------------------
// Round result
// --------------------------------------------------

inline RoundResult getRoundResult()
{
return roundResult();
}

// --------------------------------------------------
// AVA difficulty
// --------------------------------------------------

inline AvaDifficulty getAvaDifficulty()
{
return avaDifficulty();
}

inline void setAvaDifficulty(
AvaDifficulty difficulty
)
{
avaDifficulty() =
difficulty;
}

// ==================================================
// END OF AVA GAMES ENGINE
// ==================================================

} // namespace AvaGames

#endif
