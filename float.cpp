#include <iostream>    // cout, cerr, cin, endl
#include <iomanip>     // setw, setfill, etc
#include <string>      // string
#include <algorithm>   // transform
#include <string_view> // string_view
#include <cstring>     // strncpy
#include <cmath>       // float functions (isnormal, isnan, isfinite, etc)
#include <stdexcept>   // for stoi's error output

namespace
{
    /*
     * Help string.
     */
    const static std::string helpStr = R"HELP(Usage: float <flags>
Takes in data as a hexadecimal value (from standard in) and outputs its floating-point representation. 

Flags:
    Flags can be set as an argument or in stdin. To call a flag:
    -<flag character><flag arguments> (no spaces).
    
Flags include:
    -h                                    Help
    -p<number>                            Floating point precison.
    -s                                    Simple output (no table).
    -n                                    Normal out (defaults).


Return values:
    -2 if an unrecognized command line argument was found.
    -1 if an error occurred while reading from stdin.
     0 on success.
    >0 The number of inputs that were not recognized. 
)HELP";

    /*
     * String describing the last error that .
     */
    
    static std::string lastErrorMsg;
    
    /*
     * The type of input that the user has used.
     */
    enum class Input
    {
        BadInput,
        Exit,

        Float,
        Double,

        Help,
        Flag,
    };

    struct
    {
        unsigned precision    = 2;
        bool     simpleOutput = false;
        bool     printHelp    = false;
    } static currentSettings;
    
    /*
     * Representation of an IEEE 754 float, 32 or 64 bit.
     */
    template<typename T>
    class IEEE754Float
    {
    private:
        // utilizes the union trick to convert from hex to float.
        union
        {
            std::uint64_t _uint;  // 64_t to ensure it is as large as a double.
            T             _float; // float type
        };

        static constexpr std::size_t maxSizeOfBinBuffer = (sizeof(_uint) * 8);

        // string representing the _float's bits (1's or 0's).
        using oCString = char[maxSizeOfBinBuffer + 1];

        /* largely a wrapper around oCString type so it can be returned from
           a function, as arrays cannot be returned in C, but structs can. */
        struct outputString
        {
            oCString str = { '\0' };  // string representation _uint (binary)
            unsigned strSize = 0;     // how many bytes were actually used.
            ::Input  inputType;
            
            char& operator[](unsigned index)
            {
                return str[index];
            }

            outputString operator=(outputString &ostr)
            {
                std::strncpy(this->str, ostr.str, ostr.strSize);
                this->strSize = ostr.strSize;
                this->inputType = ostr.inputType;
            }
        };

        outputString getBinary()
        { 
            char             *uintBuf;           /* char buffer
                                                    representation of uint */
            std::size_t       sizeofUintBuf = 0; // size of the buffer
            outputString      binBuffer;         // return value

            /* determine the size of uintBuf. this is necessary so doubles and
               floats are not confused. */
            switch(sizeof(_float))
            {
            case sizeof(double):
                uintBuf = (char*) &_uint;
                sizeofUintBuf = sizeof(double);
                binBuffer.inputType = ::Input::Double;
                break;

            case sizeof(float):
                std::uint32_t newUint = static_cast<std::uint32_t>(_uint);
                uintBuf = (char*) &newUint;
                sizeofUintBuf = sizeof(float);
                binBuffer.inputType = ::Input::Float;
                break;
            }

            // loop through each byte of the uint
            for(int j = sizeofUintBuf - 1; j >=  0; j--)
            {
                char curByte = uintBuf[j];
                // get each bit from _uint bytes
                for(int i = (sizeof(curByte) * 8) - 1; i >= 0; i--)
                {
                    char curChar;

                    if(curByte & (1u << i))
                    {
                        curChar = '1';
                    }
                    else
                    {
                        curChar = '0';
                    }

                    binBuffer[binBuffer.strSize++] = curChar;
                }
            }
            
            return binBuffer;
        }

    public:
        static IEEE754Float<T> HexStrToIEEEFloat(const std::string_view hex)
        {
            std::istringstream hexVal(hex.data());
            std::uint64_t      inputAsHex;
            IEEE754Float<T>    floatVal;

            hexVal >> std::hex >> inputAsHex;
            floatVal = inputAsHex;

            return floatVal;
        }

        
        IEEE754Float() = default;
        
        IEEE754Float(unsigned i)
            : _uint(i)
        {
        }

        IEEE754Float<T> operator=(std::uint64_t i)
        {
            _uint = i;
            return *this;
        }

        void PrintFormattedOutput()
        {
            outputString      os = getBinary();
            unsigned          exponentSize;
            unsigned          tableSize;

            switch(os.inputType)
            {
            case ::Input::Float:
                exponentSize = 8;
                tableSize = 45;
                break;

            case ::Input::Double:
                exponentSize = 11;
                tableSize = 77;
                break;

            default:
                return; // TODO something has gone horribly wrong...
                break;
            }
            
            // prints the top and bottom of the table
            auto printEnds = [&]()
                                 {
                                     std::cout << std::setw(tableSize)
                                               << std::setfill('=')
                                               << '\n';
                                 };

            char columnStr[] = "||";
            char signStr[] = " Sign";
            char expStr[] = "Exponent";

            printEnds();
            
            std::cout << columnStr << signStr << columnStr << std::setfill(' ')  << std::setw(exponentSize) << expStr << columnStr
                      << std::setw(os.strSize - (exponentSize + 1)) << "Mantissa" << columnStr
                      << std::endl;
            
            std::cout << columnStr << std::setfill(' ')
                      << std::setw((sizeof(signStr) - 1) + (sizeof(columnStr) - 1) - sizeof(columnStr) + 1)
                      << os[0] << columnStr << std::flush;

            // print the string's exponent [1, exponentsize]
            for(unsigned i = 1; i <= exponentSize; i++)
            {
                std::cout << os[i];
            }

            std::cout << columnStr;
            // print the mantissa (fraction) (exponentsize, end]
            for(unsigned i = exponentSize + 1; i < os.strSize; i++)
            {
                std::cout << os[i];
            }

            std::cout << columnStr << std::endl;
            printEnds();
        }

        T GetIEEEFloat()
        {
            return _float;
        }

        ~IEEE754Float() = default;

    };

    // aliases
    using Float  = IEEE754Float<float>;
    using Double = IEEE754Float<double>;

    // TODO maybe do long double.
    
    /*
     * Determine if the input was a float or double. If it was neither, return BadInput.
     */
    static ::Input IsFloatOrDouble(const std::string_view str)
    {
        constexpr std::size_t floatSize = sizeof(float) * 2;
        constexpr std::size_t doubleSize = sizeof(double) * 2;
        ::Input               result = Input::BadInput;

        auto check = [&]() -> bool
        {
            constexpr char beginHexAscii = 65; // A
            constexpr char endHexAscii   = 70; // F

            for(char c : str)
            {
                // return false if char is not alphanumeric and in the Hex char range.
                if(!std::isdigit(c))
                {
                    if(c < beginHexAscii
                       || c > endHexAscii)
                    {
                        return false;
                    }
                }
            }

            return true;
        };


        switch(str.size())
        {
        case 0 ... floatSize:
            if(check())
            {
                result = ::Input::Float;
            }
            break;

        case (floatSize + 1) ... doubleSize:
            if(check())
            {
                result = ::Input::Double;
            }
            break;
        }

        return result;
    }

    /* Interprets the user's flags, and sets the mode accordingly.
       Returns false if it could not set the mode, true otherwise. */
    static bool InterpretMode(const std::string_view input)
    {
        bool success = true;
        
        switch(input[1])
        {
        case 's':
        case 'S':
            ::currentSettings.simpleOutput = true;
            break;

        case 'n':
        case 'N':
            ::currentSettings.simpleOutput = false;
            break;

        case 'h':
        case 'H':
            ::currentSettings.printHelp = true;
            break;

        case 'p':
        case 'P': {
            if(input.size() < 3)
            {
                ::lastErrorMsg = "Precision was not set with value.";
                success = false;
                break;
            }

            // get the rest of the input, and try to convert it to a string
            std::string_view numString = input.substr(2, input.size());

            try
            {
                ::currentSettings.precision =  std::stoi(numString.data());
            }
            catch(const std::invalid_argument &e)
            {
                ::lastErrorMsg = "While trying to set the float precision: ";
                ::lastErrorMsg += e.what();
            }
        }
            break;

        default:
            ::lastErrorMsg = "Unrecognized input: ";
            ::lastErrorMsg += input.data();
            success = false;
            break;
        }

        return success;
    }


    /*
     * Gets program's interpretaion of the input that the user has put in.
     */
    static ::Input GetInputType(const std::string_view str)
    {
        ::Input input = ::Input::BadInput; // return value

        // quit
        if(str[0] == 'Q')
        {
            input = ::Input::Exit;
        }
        // flag
        else if(str[0] == '-')
        {
            if(str.size() < 2)
            {
                ::lastErrorMsg = "Not enough arguments for a flag.";
            }
            else if(InterpretMode(str))
            {
                input = ::Input::Flag;
            }
        }
        // a float or double
        else
        {
            input = ::IsFloatOrDouble(str);
        }

        return input;
    }
    
}

/*
 * Runs main.
 */
int main(const int argc, const char *argv[])
{
    bool        cont = true;
    int         numFailedInputs = 0; /*  main return value (unless another error
                                         occurs). number of user inputs that
                                         could not be converted into floats.  */                             
    std::string input;


    // interpret command line arguments
    if(argc != 1)
    {
        for(int i = 1; i < argc; i++)
        {
            if(!::InterpretMode(argv[i]))
            {
                std::cerr << "Error: " << argv[i] << " is not recognized.\n";
                numFailedInputs = -2;
                cont = false;
            }
            // exit immediately if help was called
            else if(::currentSettings.printHelp)
            {
                cont = false;
                std::cout << ::helpStr << '\n';
                break;
            }
        }
    }

    // main loop
    while(cont && (std::cin >> input))
    {
  
        std::transform(input.begin(), input.end(), input.begin(),
                       [](char c) -> char
                           {
                               if(std::islower(c))
                               {
                                   return std::toupper(c);
                               }

                               return c;
                           });
        // getting rid of the leading "0x" if it exists
        std::string newInput = ((input[0] == '0') && (input[1] == 'X'))
            ? input.substr(2, input.size()) : input;
        
        ::Input inputCode = ::GetInputType(newInput);
        
        switch(inputCode)
        {
        case ::Input::Double: {
            ::Double d = ::Double::HexStrToIEEEFloat(input);
            std::cout << std::setprecision(::currentSettings.precision)
                      << d.GetIEEEFloat() << '\n';
            // print the fancy output if the user has not turned it off
            if(!::currentSettings.simpleOutput)
            {
                d.PrintFormattedOutput();
            }
            break;

        }
        case ::Input::Float: {
            ::Float f = ::Float::HexStrToIEEEFloat(input);
            std::cout << std::setprecision(::currentSettings.precision)
                      << f.GetIEEEFloat() << '\n';

            if(!::currentSettings.simpleOutput)
            {
                f.PrintFormattedOutput();
            }
            break;
        }

        case ::Input::Flag:
            // do nothing, handled elsewhere.
            break;
            
        case ::Input::Help:
            // print the helpstr and exit.
            cont = false;
            std::cout << ::helpStr << '\n';
            break;

        case ::Input::BadInput:
            std::cerr << input << " is not recognized.\n";
            // print the error message if one was set
            if(!::lastErrorMsg.empty())
            {
                std::cerr << ::lastErrorMsg << '\n';
            }
            // clear the error message
            ::lastErrorMsg.clear();
            numFailedInputs++;
            
            break;
            
        case ::Input::Exit:
            cont = false;
            break;
        }
    }

    // checking if there was an error in input.
    if(std::cin.bad())
    {
        std::cerr << "Error in input.\nNumber of failed inputs: "
                  << numFailedInputs << '\n';
        std::cin.clear();
        return -1;
    }
    
    return numFailedInputs;
}
