
#pragma once



namespace sorth::run_time::abi::words
{


    using RuntimeWordRegistrar = std::function<void(const std::string&, const std::string&)>;


    void register_runtime_words(RuntimeWordRegistrar& registrar);


}
