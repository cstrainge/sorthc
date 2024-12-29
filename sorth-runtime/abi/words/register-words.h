
#pragma once



namespace sorth::run_time::abi::words
{


    // Callback function supplied by the sorthc compiler to register native words with the run-time.
    //
    // The first parameter is the name of the word, and the second parameter is the name of the
    // function that implements the word.
    using RuntimeWordRegistrar = std::function<void(const std::string&, const std::string&)>;


    // Called by the sorthc compiler to pre-register all of the native words that are available to
    // the user's Forth code from the run-time..
    void register_runtime_words(RuntimeWordRegistrar& registrar);


    // Called at run-time to make the command line arguments available to the Forth program.
    void register_command_line_arguments(int argc, char* argv[]);


}
