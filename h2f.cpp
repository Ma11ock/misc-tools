#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

enum class Input
{
    BadInput,
    Exit,

    Float,
    Double,
};

// union trick
union hexCoverter2f
{
    unsigned int _uint;
    float _float;
};

union hexCoverter2d
{
    unsigned long _ulong;
    double _double;
};

float h2f(std::string &hex)
{
    hexCoverter2f hc2f;
    std::istringstream hexVal(hex);
    hexVal >> std::hex >> hc2f._uint;

    return hc2f._float;
}

double h2d(std::string &hex)
{
    hexCoverter2d hc2d;
    std::istringstream hexVal(hex);
    hexVal >> std::hex >> hc2d._ulong;

    return hc2d._double;
}

Input floatOrDouble(std::string &input)
{
    constexpr unsigned int floatSize = 8u;
    constexpr unsigned int doubleSize = 16u;

    Input result = Input::BadInput;

    auto check = [&](std::string in, bool oversized = true)
        {
            constexpr int beginNum = 48;
            constexpr int endNum   = 57;

            constexpr int beginLet = 65;
            constexpr int endLet   = 90;
            
            if(oversized)
            {
                if(!(in[0] == '0')
                   && ((in[1] == 'X')
                       || (in[1] == 'x')))
                {
                    return false;
                }
            }

            for(char c : in)
            {
                if((c < beginNum) ||
                   ((c > endNum) &&
                    (c < beginLet)) ||
                   (c > endLet))
                {
                    return false;
                }
            }

            return true;
        };

    switch(input.size())
    {
    case floatSize + 2u:
        if(check(input))
            result = Input::Float;
        break;
    case floatSize:
        if(check(input, false))
            result = Input::Float;
        break;

        
    case doubleSize + 2u:
        if(check(input))
            result = Input::Double;
        break;
    case doubleSize:
        if(check(input))
            result = Input::Double;
        break;
        
    default:
        break;
    }

    return result;
}

Input parseInput(std::string &input)
{
    constexpr unsigned int minSize = 4u;

    if(input.size() <= minSize)
           return Input::BadInput;
    
    else if(input == "quit")
        return Input::Exit;


    return floatOrDouble(input);
}


char change_case (char c)
{
    if (std::isupper(c)) 
        return std::tolower(c); 
    else
        return std::toupper(c); 
}

/**

 */ 
int main(int argc, char **argv)
{
    bool shouldQuit = false;

    std::string input;
    while(!shouldQuit && (std::cin >> input))
    {
        Input inputCode;
        std::transform(input.begin(), input.end(), input.begin(), change_case);

        inputCode = parseInput(input);

        switch (inputCode)
        {
        case Input::Float: 
            std::cout << h2f(input) << '\n';
            break;

        case Input::Double:
            std::cout << h2d(input) << '\n';
            break;

        case Input::Exit:
            shouldQuit = true;
            break;

        case Input::BadInput:
            std::cerr << "Error: the input " << input << " is invalid\n";
            break;
            
        default:
            break;
        }
    }
    return 0;
}
