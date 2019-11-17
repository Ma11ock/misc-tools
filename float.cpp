#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <string_view>
#include <cctype>
#include <cstring>

namespace
{
    /*
     * Help string.
     */
    const static std::string helpStr = R"HELP(Usage: float <flags>
Takes in data as a hexadecimal value (from standard in) and outputs its 
floating-point representation. 
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

        Mode
    };

    /*
     * The current mode, or way to interpret input.
     */
    enum class Mode
    {
        Help,
        
    };

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
        
        using oCString = char[maxSizeOfBinBuffer + 1];

        struct outputString
        {
            oCString str = { '\0' };  // string representation _uint
            unsigned strSize = 0;     // how many bytes were actually used.

            char& operator[](unsigned index)
            {
                return str[index];
            }

            outputString operator=(outputString &ostr)
            {
                std::strncpy(this->str, ostr.str, ostr.strSize);
                this->strSize = ostr.strSize;
            }
        };

        outputString getBinary()
        { 
            char             *uintBuf;           /* char buffer
                                                    representation of uint */
            std::size_t       sizeofUintBuf = 0; // size of the buffer
            outputString      binBuffer;         // return value

            // determine the size of uintBuf
            switch(sizeof(_float))
            {
            case sizeof(double):
                uintBuf = (char*) &_uint;
                sizeofUintBuf = sizeof(double);
                break;

            case sizeof(float):
                std::uint32_t newUint = static_cast<std::uint32_t>(_uint);
                uintBuf = (char*) &newUint;
                sizeofUintBuf = sizeof(float);
                break;
            }

            // 
            for(int j = sizeofUintBuf - 1; j >=  0; j--)
            {
                char curByte = uintBuf[j];
                // get each bit from _uint
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
            unsigned           inputAsHex;
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

        IEEE754Float<T> operator=(unsigned i)
        {
            _uint = i;
            return *this;
        }

        void PrintFormattedOutput()
        {
            outputString os = getBinary();

        }

        T GetIEEEFloat()
        {
            return _float;
        }

        ~IEEE754Float() = default;

    };

    // aliases
    using Float =  IEEE754Float<float>;
    using Double = IEEE754Float<double>;

    // TODO maybe do long double.
    
    /*
     * Converts char to uppercase if it is a letter.
     */
    char ToUpper(char c)
    {
        if(std::islower(c))
        {
            return std::tolower(c);
        }

        return c;
    }

    /*
     * Determine if the input was a float or double. If it was neither, return BadInput.
     */
    ::Input IsFloatOrDouble(const std::string_view str)
    {
        constexpr unsigned floatSize = 8;
        constexpr unsigned doubleSize = 16;

        // make sure that the size of the input is at least 4 chars.
        if(str.size() < 4)
        {
            ::lastErrorMsg = "The input was not correctly sized.";
            return ::Input::BadInput;
        }

        ::Input result = Input::BadInput;

        auto check =[&](bool oversized = true) -> bool
        {
            constexpr char beginHexAscii = 65; // A
            constexpr char endHexAscii   = 70; // F
            
            if(oversized)
            {
                // make sure the first two chars are 0X
                if(!(str[0] == '0')
                   && ((str[1] == 'X')))
                {
                    return false;
                }
            }

            for(char c : str)
            {
                // return false if char is not alphanumeric and in the Hex char range.
                if(!(std::isdigit(c) || (c < beginHexAscii || c > endHexAscii)))
                {
                    return false;
                }
            }

            return true;
        };


        switch(str.size())
        {
        case floatSize + 2: // begins with 0x
            if(check())
            {
                result = ::Input::Float;
            }
            break;

        case floatSize: // does not begin with 0x
            if(check(false))
            {
                result = ::Input::Float;
            }
            break;

        case doubleSize + 2: // begins with 0x
            if(check())
            {
                result = ::Input::Double;
            }
            break;

        case doubleSize: // does not begin with 0x
            if(check(true))
            {
                result = ::Input::Double;
            }
            break;
        }

        return result;
    }

    /*
     * Gets program's interpretaion of the input that the user has put in.
     */
    ::Input GetInputType(const std::string_view str)
    {
        ::Input input; // return value

        // quit
        if(str[0] == 'q')
        {
            input = ::Input::Exit;
        }
        // flag
        else if(str[0] == '-')
        {
//            char firstChar = str[0];
            input = ::Input::Mode;
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

    ::Double d = 0x3U;
    d.PrintFormattedOutput();

    ::Float f = 0x3U;
    f.PrintFormattedOutput();
    
    return 0;
    
    while(cont && (std::cin >> input))
    {
        Input inputCode = GetInputType(input);
        std::transform(input.begin(), input.end(), input.begin(), ::ToUpper);

        switch(inputCode)
        {
        case ::Input::Double:
            std::cout << ::Double::HexStrToIEEEFloat(input).GetIEEEFloat() << '\n';
            break;

        case ::Input::Float:
            std::cout << ::Float::HexStrToIEEEFloat(input).GetIEEEFloat() << '\n';
            break;

        case ::Input::Mode:
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
