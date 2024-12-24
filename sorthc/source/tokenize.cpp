
#include "sorthc.h"



namespace sorthc::source
{


    namespace
    {


        // Check to see if the next character is whitespace.
        bool is_whitespace(char next)
        {
            return    (next == ' ')
                   || (next == '\t')
                   || (next == '\n');
        }


        // Skip whitespace in the source buffer.  Keep going until we hit a non-whitespace character
        // or the end of the buffer.  We don't return a special code for the end of the buffer, we
        // expect that the calling code will check for this.
        void skip_whitespace(SourceBuffer& source)
        {
            char next = source.peek();

            while (   source
                   && is_whitespace(next))
            {
                source.next();
                next = source.peek();
            }
        }


        // Skip whitespace until we reach a target column.  This can skip over multiple new lines
        // which will be accounted for later.  We only return when we reach a line that has
        // whitespace up to the target column or we hit non-whitespace text, or the end of the
        // buffer.
        //
        // Used by the multi-line string processing.
        void skip_whitespace_until_column(SourceBuffer& source, size_t target_column)
        {
            char next = source.peek();

            while (   source
                   && is_whitespace(next)
                   && (source.get_location().get_column() < target_column))
            {
                source.next();
                next = source.peek();
            }
        }


        // Add a given count new lines to a string.  Used by the multi-line string processing.
        void append_newlines(std::string& text, size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                text += '\n';
            }
        }


        // Process an escaped character in a string literal.
        char process_escape_literal(SourceBuffer& source)
        {
            // Get the next character in the literal.
            char next = source.next();

            // What did we get?
            switch (next)
            {
                // Process standard escape characters.
                case 'n':
                    next = '\n';
                    break;

                case 'r':
                    next = '\r';
                    break;

                case 't':
                    next = '\t';
                    break;

                case '\\':
                    next = '\\';
                    break;

                case '"':
                    next = '"';
                    break;

                // Process the numeric escape character, convert a numeric value into a character.
                // We only handle values from 0 to 255, and we don't process hex or octal here.
                // Floating point literals are also invalid here.
                case '0':
                    {
                        Location start = source.get_location();
                        std::string number_string;

                        // Gather the numeric characters.
                        while (source)
                        {
                            next = source.peek();

                            if ((next >= '0') && (next <= '9'))
                            {
                                number_string += next;
                                source.next();
                            }
                            else
                            {
                                break;
                            }
                        }

                        // TODO: Support hex.
                        // Convert the string to a number, while making sure it's in range.
                        auto numeric = std::stoi(number_string);

                        throw_error_if(numeric >= 256,
                                       start,
                                       "Numeric character literal out of range.");

                        // Finally convert to a char.
                        next = (char)numeric;
                    }
                    break;

                // Looks like an unsupported escape character.
                default:
                    {
                        std::stringstream stream;

                        stream << "Unsupported escape character " << next << ".";

                        throw_error(source.get_location(), stream.str());
                    }
                    break;
            }

            // Return the processed character.
            return next;
        }


        // Extract a multi-line string from the source buffer.  This is a string that starts with a
        // "* and ends with a *".  The string can contain new lines and whitespace.  The string can
        // also contain escaped characters.
        //
        // Extra whitespace at the beginning of the string is ignored, but the column of the first
        // non-whitespace character is recorded.  This column is used to determine how much
        // whitespace to skip on subsequent lines.
        std::string process_multi_line_string(const Location& start, SourceBuffer& source)
        {
            // Extract the *.
            source.next();

            // Extract first whitespace until we get to first real text, and record the column we
            // are at now.
            skip_whitespace(source);

            auto target_column = source.get_location().get_column();
            char next = 0;

            std::string new_string;

            while (source)
            {
                next = source.next();

                // Found an asterisk, check to see if the next char is a quote.  If it is, we're
                // done with this string.
                if (next == '*')
                {
                    if (source.peek() == '"')
                    {
                        next = source.next();
                        break;
                    }
                    else
                    {
                        new_string += next;
                    }
                }
                else if (next == '\\')
                {
                    // Process the escaped character.
                    new_string += process_escape_literal(source);
                }
                else if (next == '\n')
                {
                    new_string += next;

                    // We're on a new line, so get rid of the whitespace until we either find text
                    // or reach the column we're looking for.  Any whitespace after that column will
                    // be included in the string.  Any skiped new lines should be added to the
                    // string.
                    auto start_line = source.get_location().get_line();

                    skip_whitespace_until_column(source, target_column);

                    auto current_line = source.get_location().get_line();

                    if (current_line > (start_line))
                    {
                        append_newlines(new_string, current_line - (start_line));
                    }
                }
                else
                {
                    // No special processing needed for this character.
                    new_string += next;
                }
            }

            return new_string;
        }


        // Process a single line string.  If the string looks like it's actually a multi-line
        // string, we'll shunt to process_multi_line_string.
        std::string process_string(SourceBuffer& source)
        {
            // Record the start location of the string, we'll take the position of the opening quote
            // as the start of the string.
            Location start = source.get_location();

            std::string new_string;

            // Eat the quote character.
            source.next();

            // Is the very next character a *?  If it is, then we're looking at a multi-line string.
            if (source.peek() == '*')
            {
                // Process as a multi-line string literal.
                new_string = process_multi_line_string(start, source);
            }
            else
            {
                // Process as a single line string literal.
                char next = 0;

                // Gather the text until we find the closing quote or hit the end of the buffer.
                while (source)
                {
                    next = source.next();

                    if (next == '"')
                    {
                        break;
                    }

                    throw_error_if(next == '\n', start, "Unexpected new line in string literal.");

                    if (next == '\\')
                    {
                        next = process_escape_literal(source);

                    }
                    new_string += next;
                }

                // We should have found the closing quote, if we didn't, then we have a problem.
                throw_error_if(next != '"', start, "Missing end of string literal.");
            }

            // Return the gathered text.
            return new_string;
        }


        // Get characters while the test returns true.  Condense the characters into a string and
        // return the string.
        std::string get_while(SourceBuffer& source, std::function<bool(char)> test) noexcept
        {
            std::string new_string;

            // Peek at the next character for the loop.
            char next = source.peek();

            // Gather that text.
            while (    (source)
                    && (test(next)))
            {
                // Extract the character from the buffer and peek at the next character.
                new_string += source.next();
                next = source.peek();
            }

            // Return the gathered text.
            return new_string;
        }


        // Check to see if the text looks like it's a number, we're not doing a deep check here.
        // It'll be resolved properly when we try to convert it to a number.
        bool looks_numeric(const std::string& text) noexcept
        {
            if (text.empty())
            {
                return false;
            }

            if ((text[0] >= '0') && (text[0] <= '9'))
            {
                return true;
            }

            if (   (text[0] == '-')
                || (text[0] == '+'))
            {
                return (text[1] >= '0') && (text[1] <= '9');
            }

            return false;
        }


        // Check to see if the text looks like an integer value.
        bool is_all_digits(const std::string& text) noexcept
        {
            size_t start = 0;

            // Check to see if the first character is a sign.  If it is, then we'll start at the
            // next character.
            if ((text[0] == '-') || (text[0] == '+'))
            {
                start = 1;
            }

            // Check to see if all the characters are digits.  If any aren't, bail out.
            for (size_t i = start; i < text.size(); ++i)
            {
                const char& next = text[i];

                if ((next < '0') || (next > '9'))
                {
                    return false;
                }
            }

            // Looks like all the characters are digits.
            return true;
        }


        // Try to convert the text into a number token.  If the text can't be converted into a
        // number, then we'll return a word token.
        Token try_make_number_token(const std::string& text, const Location& location)
        {

            // Remove any _ separators from the text.
            auto filtered_text = text;
            filtered_text.erase(std::remove(filtered_text.begin(), filtered_text.end(), '_'),
                                filtered_text.end());

            Token new_token;

            // Try to convert the text into a number.  We do a quick check to see if it looks like a
            // floating point number.  Otherwise we'll see if it's a binary or hex number.  If it's
            // none of those, then we'll try to convert it into a decimal number.  If there are non
            // numeric characters in the text, then we'll treat it as a word token.
            try
            {
                if (filtered_text.find('.') != std::string::npos)
                {
                    double value = std::stod(filtered_text);
                    new_token = Token(location, value);
                }
                else if ((filtered_text[0] == '0') && (filtered_text[1] == 'x'))
                {
                    int64_t value = (int64_t)std::stoll(filtered_text, nullptr, 16);
                    new_token = Token(location, value);
                }
                else if ((filtered_text[0] == '0') && (filtered_text[1] == 'b'))
                {
                    int64_t value = (int64_t)std::stoll(filtered_text.substr(2), nullptr, 2);
                    new_token = Token(location, value);
                }
                else if (is_all_digits(text))
                {
                    int64_t value = (int64_t)std::stoll(filtered_text);
                    new_token = Token(location, value);
                }
                else
                {
                    // Use the original unfiltered text for a new word token.
                    new_token = Token(location, Token::Type::word, text);
                }
            }
            catch (...)
            {
                // Something went wrong during the conversion, use the original unfiltered text for
                // a new word token.
                new_token = Token(location, Token::Type::word, text);
            }

            // Return the new token.
            return new_token;
        }


    }


    TokenList tokenize_source(SourceBuffer& source)
    {
        TokenList tokens;

        tokens.reserve(1024);

        // While there is text to get from the buffer...
        while (source)
        {
            // Skip any whitespace, and check to see if there's anything left to process.
            skip_whitespace(source);

            if (!source)
            {
                break;
            }

            // We still have text to process, so get the next token.
            Location location = source.get_location();

            Token next_token;
            std::string text;

            // If the next character is a quote, then we're looking at a string literal.
            if (source.peek() == '"')
            {
                // Process as either a single or multi-line string literal.
                text = process_string(source);
                next_token = Token(location, Token::Type::string, text);
            }
            else
            {
                // Get the next words worth of text.
                text = get_while(source,
                    [&](char next) -> bool
                    {
                        return is_whitespace(next) == false;
                    });

                // Check to see if the text looks like a number.
                if (looks_numeric(text))
                {
                    // Try to process the text as a number, it may still fail and we get a word
                    // token.
                    next_token = try_make_number_token(text, location);
                }
                else
                {
                    // Looks like we have a word.
                    next_token = Token(location, Token::Type::word, text);
                }
            }

            // Add the token to the list.
            tokens.push_back(next_token);
        }

        // Return the gathered list.
        return tokens;
    }


}
