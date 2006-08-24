#include "StringToNumber.h"

using namespace monash;

StringToNumber::StringToNumber()
{
}

StringToNumber::~StringToNumber()
{
}

std::string StringToNumber::toString()
{
    return "procedure:number?";
}

Object* StringToNumber::eval(Environment* env)
{
    printf("don't eval me");
    return NULL;
}

Object* StringToNumber::apply(Objects* arguments)
{
    if (arguments->size() != 1)
    {
        printf("string->number got error");
        return NULL;
    }
    Object* o = arguments->at(0);
    if (o->type() != Object::STRING) return new Number(0);
    String* str = (String*)o;
    std::string text = str->value();
    for (int i = 0; i < text.length(); i++)
    {
        if (!isdigit(text[i]))
        {
            return new Number(0);
        }
    }
    return new Number(atoi(text.c_str()));
}