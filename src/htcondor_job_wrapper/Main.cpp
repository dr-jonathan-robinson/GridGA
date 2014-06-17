#include "stdafx.hpp"

namespace po = boost::program_options;

//______________________________________________________________________________________________________________
//
//struct ArgTypeInt
//    : public boost::program_options::typed_value<int>
//{
//    ArgTypeInt(const std::string& name)
//        : boost::program_options::typed_value<int>(&my_value)
//        , my_name(name)
//        , my_value(0)
//    {
//    }
//    std::string name() const { return my_name; }
//    std::string my_name;
//    int my_value;
//};
//
////______________________________________________________________________________________________________________
//
//struct ArgTypeString
//    : public boost::program_options::typed_value<std::string>
//{
//    ArgTypeString(const std::string& name)
//        : boost::program_options::typed_value<std::string>(&my_value)
//        , my_name(name)
//        , my_value("")
//    {
//    }
//    std::string name() const { return my_name; }
//    std::string my_name;
//    std::string my_value;
//};
//
////______________________________________________________________________________________________________________
//
//struct ArgTypeDouble
//    : public boost::program_options::typed_value<double>
//{
//    ArgTypeDouble(const std::string& name)
//        : boost::program_options::typed_value<double>(&my_value)
//        , my_name(name)
//        , my_value(0.0)
//    {
//    }
//    std::string name() const { return my_name; }
//    std::string my_name;
//    double my_value;
//};

//______________________________________________________________________________________________________________

void TransmitToGAServer(std::string backtestResults, std::string serverName)
{
    std::cout << backtestResults << std::endl;
    zmq::context_t zmqContext(1);

    std::size_t attemptCount = 1;
    bool sentOk = false;

    std::size_t secondsToWait = 10;

    while (!sentOk && attemptCount < 11)
    {
        std::cout << "Sending Attempt " << attemptCount << " to " << serverName << std::endl;
        try
        {
            zmq::socket_t sendSocket(zmqContext, ZMQ_REQ);
            // this is required due to a bug in zeromq which causes the app to hang when the context is terminated
            int linger = 0;
            sendSocket.setsockopt (ZMQ_LINGER, &linger, sizeof (linger));

            sendSocket.connect(serverName.c_str());
            zmq::message_t sendMessage(backtestResults.length());
            memcpy ((void *) sendMessage.data(), backtestResults.c_str(), backtestResults.length());  

            sendSocket.send(sendMessage);

            // wait for a response
            zmq::pollitem_t items[] = 
            {
                {sendSocket, 0, ZMQ_POLLIN, 0}
            };

            zmq::poll(items, 1, secondsToWait * 1000);

            if (items[0].revents & ZMQ_POLLIN)
            {
                zmq::message_t message;
                sendSocket.recv(&message);
                char* receivedChar = static_cast<char*>(malloc(message.size() + 1));
                memcpy(receivedChar, message.data(), message.size());
                receivedChar[message.size()] = 0;
                std::string receivedString(receivedChar);
                std::cout << "Received response " << receivedString;
                if (receivedString.compare("_ok_") == 0)
                {
                    sentOk = true;
                }
            }
            else
            {
                std::cerr << "Could not transmit results to GA server. Attempt " << attemptCount << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception while sending : " << e.what() << std::endl;
        }
        ++attemptCount;
    }
}

//______________________________________________________________________________________________________________

void SendError(std::string error, std::string id, std::string serverName)
{
    std::cerr << error << std::endl;
    std::ostringstream s;
    s <<         
        "<results>" << std::endl <<
        "    <id>" << id << "</id>" << std::endl <<
        "    <objective>-1</objective>" << std::endl <<
        "    <error>" << error << "</error>" << std::endl <<
        "</results>";
    TransmitToGAServer(s.str(), serverName);
}

//______________________________________________________________________________________________________________

int main(int argc, char* argv[])
{
    std::string configFileName(argv[1]);
    // Check the config file exists
    if (!boost::filesystem::exists(configFileName))
    {
        std::cerr << "Error cannot find config file: " << configFileName << std::endl;
        return 1;
    }

    // Read the config
    boost::property_tree::ptree pt;
    try
    {
        read_xml(configFileName, pt);
    }
    catch (std::exception& e)
    {
        std::cerr << __FUNCTION_NAME__ << "Cannot load config. Check for XML errors. The error was " << e.what() << std::endl;
        return -1;
    }

    std::string executeCmd = CommonLib::GetOptionalParameter<std::string>("config.execute", pt, "NONE");
    std::string objCmd = CommonLib::GetOptionalParameter<std::string>("config.extract-obj-value", pt, "NONE");
    std::string server = CommonLib::GetOptionalParameter<std::string>("config.server", pt, "NONE");
    std::string genomeID = CommonLib::GetOptionalParameter<std::string>("config.genome-id", pt, "NONE");

    if (boost::iequals(executeCmd, "NONE"))
    {
        SendError("execute command has not been supplied!", genomeID, server);
        return 1;
    }
    executeCmd = executeCmd + " > std.out 2>&1";
    std::system(executeCmd.c_str());

    if (!boost::iequals(objCmd, "NONE"))
    {
        objCmd = objCmd + " > obj.out 2>&1";
        std::system(objCmd.c_str());
    }

    if (!boost::filesystem::exists("obj.out"))
    {
        SendError("Could not find the value of the objective function (obj.out)!", genomeID, server);
        return 1;
    }

    std::ifstream inFile;
    inFile.open("obj.out");
    std::string objValue;
    std::getline(inFile, objValue);
    inFile.close();

    // write the out
    std::ostringstream sendXML;
    sendXML << 
        "<results>" << std::endl <<
        "    <id>" << genomeID << "</id>" << std::endl <<
        "    <objective>" << objValue << "</objective>" << std::endl <<
        "</results>";

    TransmitToGAServer(sendXML.str(), server);

    return 0;
}

