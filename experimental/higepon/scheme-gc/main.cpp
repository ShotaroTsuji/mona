/*!
  \file   main.cpp
  \brief  scheme

  Copyright (c) 2006 Higepon
  WITHOUT ANY WARRANTY

  \author  Higepon
  \version $Revision$
  \date   create:2006/06/16 update:$Date$
*/
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "scheme.h"
#include <stdio.h>
#include <time.h>
#ifdef MONA
#include <monapi.h>
#else
#include <sys/time.h>    // rlimit
#include <sys/resource.h> // rlimit
#endif
#include "ExtRepParser.h"
using namespace util;
using namespace monash;

uint32_t count_char(const char* s, char c)
{
    int length = strlen(s);
    uint32_t count = 0;
    for (int i = 0; i < length; i++)
    {
        if (s[i] == c) count++;
    }
    return count;
}

void input_loop()
{
    QuoteFilter quoteFilter;
    MacroFilter f;
    Translator translator;
    Environment* env = new Environment(f, translator);
    SCM_ASSERT(env);
    g_top_env = env;
    registerPrimitives(env);

#ifdef MONA
    char line[1024];
#else
    char* line = NULL;;
    size_t length = 0;
#endif

    uint32_t open_paren_count = 0;
    uint32_t close_paren_count = 0;
    bool show_prompt = true;

    RETURN_ON_ERROR("stdin");
    String input = "(load \"test/scheme.scm\")";

    input = quoteFilter.filter(input);

    input = "(" + input + " )";
    SExp* allSExp = SExp::fromString(input);
    SExps sexps = allSExp->sexps;

    for (int i = 0; i < sexps.size(); i++)
    {
        SExp* sexp = sexps.get(i);

        f.filter(sexp);
        Object* object = NULL;
        translator.translate(&sexp, &object);
        object->eval(env);

    }
    input = "";

    for (;;)
   {

        if (show_prompt) SCHEME_WRITE(stdout, "mona> ");
#ifdef MONA
        monapi_stdin_read((uint8_t*)line, 1024);
#else
        getline(&line, &length, stdin);
#endif
        open_paren_count += count_char(line, '(');
        close_paren_count += count_char(line, ')');
        input += line;
        if (input != "" && open_paren_count == close_paren_count)
        {
            input = quoteFilter.filter(input);
            TRANSCRIPT_WRITE(input.data());

#if 1
            StringReader* reader = new StringReader(input);
            Scanner* scanner = new Scanner(reader);
            ExtRepParser parser(scanner);
            Object* evalFunc = (new Variable("eval"))->eval(env);
            for (Object* sexp = parser.parse(); sexp != SCM_EOF; sexp = parser.parse())
            {
                Object* evaluated;
                SCM_EVAL(evalFunc, env, evaluated, sexp);
                SCHEME_WRITE(stdout, "%s\n", evaluated->toString().data());
            }

#else
            input = String("(") + input + " )";
            SExp* allSExp = SExp::fromString(input);
            SExps sexps = allSExp->sexps;
            for (int i = 0; i < sexps.size(); i++)
            {
                SExp* sexp = sexps.get(i);
                f.filter(sexp);

                Object* object = NULL;

                if (translator.translate(&sexp, &object) != Translator::SUCCESS)
                {
//                    fprintf(stderr, "translate error \n");
                    open_paren_count = 0;
                    close_paren_count = 0;
                    input = "";
                    break;
                }
                // let's eval!
                Object* evaluated = object->eval(env);
                SCHEME_WRITE(stdout, "%s\n", evaluated->toString().data());

            }
#endif
                    open_paren_count = 0;
                    close_paren_count = 0;
                    show_prompt = true;
                    input = "";

        }
        else
        {
            show_prompt = false;
            //          printf("diff\n");
//        printf("%s[%d]\n", line, length);
        }

    }

}

int main(int argc, char *argv[])
{
#ifdef USE_MONA_GC
    gc_init();
#endif

#ifndef MONA
    struct rlimit r;
    getrlimit(RLIMIT_STACK, &r);
    r.rlim_cur = 64 * 1024 * 1024;

    setrlimit(RLIMIT_STACK, &r);
    getrlimit(RLIMIT_STACK, &r);
#endif

    cont_initialize();
    if (argc == 1)
    {
        input_loop();
        return 0;
    }

    String input = load(argv[1]);
    if (input == "")
    {
        fprintf(stderr, "can not load: %s file\n", argv[1]);
        return -1;
    }

    QuoteFilter quoteFilter;
    input = quoteFilter.filter(input);
    Error::exitOnError();
    Error::file = argv[1];

    MacroFilter f;
    Translator translator;
    Environment* env = new Environment(f, translator);SCM_ASSERT(env);
    g_top_env = env;
    registerPrimitives(env);

#if 1
    StringReader* reader = new StringReader(input);
    Scanner* scanner = new Scanner(reader);
    ExtRepParser parser(scanner);
    Object* evalFunc = (new Variable("eval"))->eval(env);
    for (Object* sexp = parser.parse(); sexp != SCM_EOF; sexp = parser.parse())
    {
        Object* evaluated;
        SCM_EVAL(evalFunc, env, evaluated, sexp);
    }
#else

    input = "(" + input + " )";
    SExp* allSExp = SExp::fromString(input);

    SExps sexps = allSExp->sexps;

    for (int i = 0; i < sexps.size(); i++)
    {
        SExp* sexp = sexps.get(i);
        f.filter(sexp);

        Object* object = NULL;

        if (translator.translate(&sexp, &object) != Translator::SUCCESS)
        {
            SCHEME_WRITE(stderr, "translate error \n");
            return -1;
        }
        // let's eval!
        object->eval(env);

    }
#endif
    return 0;
}
