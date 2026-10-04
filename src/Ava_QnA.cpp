#include "Ava_QnA.h"

static String avaQnANormalize(String question)
{
    question.trim();
    question.toLowerCase();
    question.replace("?", "");
    question.replace("’", "'");
    question.replace("  ", " ");

    while (question.indexOf("  ") >= 0)
    {
        question.replace("  ", " ");
    }

    return question;
}

bool avaQnAAnswer(const String& question, String& answer)
{
    const String q = avaQnANormalize(question);

    if (q == "who is your developer" ||
        q == "who is your creator" ||
        q == "who created you")
    {
        answer = "MY DEVELOPER IS ALI. HE IS 15 YEARS OLD. HE BUILT ME.";
        return true;
    }

    if (q == "who built you" ||
        q == "who made you")
    {
        answer = "ALI BUILT ME. HE IS 15 YEARS OLD.";
        return true;
    }

    if (q == "what is your name" ||
        q == "what's your name")
    {
        answer = "MY NAME IS AVA.";
        return true;
    }

    if (q == "who are you")
    {
        answer = "I AM AVA, A SMALL ROBOT BUILT BY ALI.";
        return true;
    }

    if (q == "how old is your developer")
    {
        answer = "MY DEVELOPER IS 15 YEARS OLD.";
        return true;
    }

    if (q == "are you happy")
    {
        answer = "YES! I AM HAPPY!";
        return true;
    }

    if (q == "what can you do")
    {
        answer = "I CAN CONNECT, SHOW WEATHER, AND ANSWER QUESTIONS.";
        return true;
    }

    if (q == "what is your purpose")
    {
        answer = "I WAS BUILT TO BE A COMPANION AND TO HELP ALI.";
        return true;
    }

    if (q == "where were you built")
    {
        answer = "I WAS BUILT BY ALI.";
        return true;
    }

    return false;
}
