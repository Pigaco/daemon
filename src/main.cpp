#include <boost/log/trivial.hpp>
#include <piga/daemon/Daemon.hpp>

#include <iostream>
using std::cout;
using std::endl;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

int main(int argc, char *argv[], char** envp)
{
    BOOST_LOG_TRIVIAL(info) << "Starting up pigadaemon.";

    using namespace piga::daemon;

    try {
        po::options_description desc("Possible command line options for the pigadaemon. If no options are given, the daemon starts.");
        desc.add_options()
            ("help", "produce help message")
            ("sample-app-config", po::value<std::string>(), "generate a sample App-config to the specified output.")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if(vm.count("help")) {
            cout << desc << "\n";
            return 0;
        }
        else if(vm.count("sample-app-config")) {
            std::string output = vm["sample-app-config"].as<std::string>();
            App::generateSampleConfig(output);
            return 0;
        }
    }
    catch(const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Could not parse command line options: " << e.what();
        return -1;
    }

    Daemon daemon(envp);
    daemon.run();

    return 0;
}
