
#pragma once



namespace sorth::run_time::abi::words
{


    using RuntimeWordRegistrar = std::function<void(const std::string&, const std::string&)>;


    void register_runtime_words(RuntimeWordRegistrar& registrar);


    // Called at run-time to make the command line arguments available to the Forth program.
    void register_command_line_arguments(int argc, char* argv[]);


}
