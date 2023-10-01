#include <variant>
#include <iostream>
#include <ostream>
#include <string>
#include <optional>
#include <unordered_map>
#include <vector>
#include <algorithm>

template <typename T>
struct CmdlineArgRef {
  std::string key;
  T value;
};

using AllowedArgTypes = std::variant<int, bool, float, std::string>; // we can extent this to support more types

//currently we only support "--xx" or "-x"
std::string parseKey(const std::string & arg) {
    if (arg.substr(0, 2) == "--") {
      return arg.substr(2);
    } else if(arg.substr(0, 1) == "-") {
      return arg;
    }
     throw std::runtime_error("parse invalid args: " + arg);
  }

struct Argument {
    std::optional<std::string> value;  // Change value type to optional<string>
    std::string description;
    bool default_value = false;
    bool is_store_true = false; // Add a new field to indicate whether the argument is store_true
    bool is_store_passed = false; // Add a new field to indicate whether the argument is passed
};

struct ArgsParser {
  std::unordered_map<std::string, Argument> requeiredArguments;
  std::unordered_map<std::string, Argument> optionalArguments;
  int num_optional_args = 0;
  int pass_optional_args = 0;
};

template <typename T>
  CmdlineArgRef<T> add_required_argument(ArgsParser & parser, const std::string & key, const std::optional<T> & default_value,
                         const std::string &description, bool is_store_true = false) {
    std::string parse_key = parseKey(key);
    parser.requeiredArguments[parse_key].description = description;
    if(default_value.has_value()) {  // Use has_value() to check if there's a value
        parser.requeiredArguments[parse_key].value = std::to_string(default_value.value());  // Convert the value to string
        parser.requeiredArguments[parse_key].default_value = true;
        parser.requeiredArguments[parse_key].is_store_true = is_store_true;
        return CmdlineArgRef<T>{parse_key, default_value.value()};
    } 
    return CmdlineArgRef<T>{parse_key, T{}};
  }

//default_value is std::nullopt 
template <typename T>
  CmdlineArgRef<T> add_optional_argument(ArgsParser & parser, const std::string & key, const std::optional<T> & default_value,
                         const std::string &description, bool is_store_true = false) {
    std::string parse_key = parseKey(key);
    parser.optionalArguments[parse_key].description = description;
    parser.optionalArguments[parse_key].is_store_true = is_store_true;
    parser.num_optional_args++;
    return CmdlineArgRef<T>{parse_key, T{}};
  }

template <typename T>
  T convert(std::string const &s) ;

template <>
int convert<int>(std::string const &s)  {
  return std::stoi(s);
}

template <>
float convert<float>(std::string const &s)  {
  return std::stof(s);
}

template <>
bool convert<bool>(std::string const &s) {
  return s == "true" || s == "1" || s == "yes";
}

ArgsParser parse_args(const ArgsParser & mArgs, int argc, const char **argv) {
    int i  = 1;
    ArgsParser result;
    std::vector<std::string> optional_args_passed;
   for(const auto & [key, arg] : mArgs.requeiredArguments) {
    result.requeiredArguments[key] = arg;
  }
  for(const auto & [key, arg] : mArgs.optionalArguments) {
    result.optionalArguments[key] = arg;
  }
  result.num_optional_args = mArgs.num_optional_args;
    while (i  < argc) {
        std::string key = parseKey(argv[i]);
        if (key == "help" || key == "h") {
            exit(1);
        }

        if(mArgs.requeiredArguments.count(key) && mArgs.requeiredArguments.at(key).is_store_true) {
            result.requeiredArguments[key].value = "true";
            result.requeiredArguments[key].is_store_true = true;
            result.requeiredArguments[key].is_store_passed = true;
            i++;
            continue; 
        }

        if (i + 1 < argc && argv[i + 1][0] != '-') {
            if(result.requeiredArguments.count(key)) {
                result.requeiredArguments[key].value = argv[i + 1];
            } else if(result.optionalArguments.count(key)) {
                result.optionalArguments[key].value = argv[i + 1];
                result.pass_optional_args++;
                optional_args_passed.push_back(key);
            } else {
                throw std::runtime_error("invalid args: " + key + " does not exist") ;
            }
            i += 2; 
        } else {
            if (result.requeiredArguments.count(key) && !result.requeiredArguments.at(key).is_store_true) {
                throw std::runtime_error("required args: " + key + " needs a value");
            }
            i++; 
        }
    }
    std::cout<<"result.pass_optional_args:"<<result.pass_optional_args<<" and  result.num_optional_args:"<<result.num_optional_args<<std::endl;
    if(result.pass_optional_args != result.num_optional_args) {
        std::vector<std::string> missing_args;
        for(const auto & [key, arg] : mArgs.optionalArguments) {
            if(std::find(optional_args_passed.begin(), optional_args_passed.end(), key) == optional_args_passed.end()) {
                missing_args.push_back(key);
            }
        }
        std::string missing_args_str = "";
        for(const auto & arg : missing_args) {
            missing_args_str +=  arg + "  " ;
        }
        throw std::runtime_error("some optional args are not passed: " + missing_args_str);
    }

    return result;
  }

template <typename T>
T get(const ArgsParser & parser , const CmdlineArgRef<T> &ref)  {
    std::string key = ref.key;
    if(parser.requeiredArguments.count(key)) {
        if(parser.requeiredArguments.at(key).is_store_true) {
            if(parser.requeiredArguments.at(key).is_store_passed) {
                return true;
            } else {
                return false;
            }
        } else if (parser.requeiredArguments.at(key).default_value || parser.requeiredArguments.at(key).value.has_value()) {
            return convert<T>(parser.requeiredArguments.at(key).value.value());
        } 
    } else if(parser.optionalArguments.count(key)) {
        if (parser.optionalArguments.at(key).default_value || parser.optionalArguments.at(key).value.has_value()) {
            return convert<T>(parser.optionalArguments.at(key).value.value());
        }
    }
    throw std::runtime_error("invalid args: " + ref.key);
}


int main() {

      // char const *test_argv[] = {"program_name",
      //                        "--batch-size",
      //                        "100",
      //                        "--fusion",
      //                        "false",
      //                        "--verbose"};
    // char const *test_argv[] = {"program_name",
    //                          "--batch-size",
    //                          "100",
    //                           "-ll:gpus",
    //                          "6",
    //                          "-ll:cpus",
    //                           "8",
    //                          "--fusion",
    //                          "false",
    //                          "--verbose"};

        // char const *test_argv[] = {"program_name",
        //                      "--batch-size",
        //                      "100",
        //                       "-ll:gpus",
        //                      "6",
        //                      "-ll:cpus",
        //                       "8",
        //                      "--fusion",
        //                      "false"};

            char const *test_argv[] = {"program_name",
                             "--batch-size",
                             "100",
                             "--thx",
                             "0.03",
                              "--learning-rate"};
    
    // char const *test_argv[] = {"program_name",
    //                          "--batch-size",
    //                          "100",
    //                           "-ll:gpus",
    //                          "6",
    //                          "--fusion",
    //                          "false"};
    
    ArgsParser args;
    auto batch_size_ref = add_required_argument(args, "--batch-size", std::optional<int>(32), "Size of each batch during training");
    auto learning_rate_ref = add_required_argument(args, "--learning-rate", std::optional<float>(0.001), "Learning rate for the optimizer");
    auto thx_ref = add_required_argument(args, "--thx", std::optional<float>(0.001), "Learning rate for the optimizer");
      constexpr size_t test_argv_length = sizeof(test_argv) / sizeof(test_argv[0]);
    ArgsParser result = parse_args(args , test_argv_length, const_cast<const char **>(test_argv));
  std::cout<<"batch_size:"<<get(result, batch_size_ref)<<std::endl;
    std::cout<<"learning_rate:"<<get(result, learning_rate_ref)<<std::endl;
    std::cout<<"thx:"<<get(result, thx_ref)<<std::endl;
    //auto ll_gpus_ref = add_required_argument<int>(args, "-ll:gpus", std::nullopt, "Number of GPUs to be used for training");
  //   auto fusion_ref = add_required_argument(args, "--fusion", std::optional<bool>(true), "Whether to use fusion or not");
  //   auto ll_gpus_ref = add_optional_argument<int>(args, "-ll:gpus", std::nullopt, "Number of GPUs to be used for training");
  //   auto ll_cpus_ref = add_optional_argument<int>(args, "-ll:cpus", std::nullopt, "Number of CPUs to be used for training");

  //   auto verbose_ref = add_required_argument(args, "--verbose", std::optional<bool>(false), "Whether to print verbose logs",true);
  //   constexpr size_t test_argv_length = sizeof(test_argv) / sizeof(test_argv[0]);
  //   ArgsParser result = parse_args(args , test_argv_length, const_cast<const char **>(test_argv));

  //  std::cout<<"batch_size:"<<get(result, batch_size_ref)<<std::endl;
  //   std::cout<<"ll_gpus:"<<get(result, ll_gpus_ref)<<std::endl;
  //   std::cout<<"fusion:"<<get(result, fusion_ref)<<std::endl;
  //   std::cout<<"ll_cpus:"<<get(result, ll_cpus_ref)<<std::endl;
  //  bool is_verbose = get(result, verbose_ref);

    //std::cout<<"verbose:"<<is_verbose<<std::endl;
  // std::cout << "batch_size: " << args.get(batch_size_ref) << std::endl;
  // std::cout << "learning_rate: " << args.get(learning_rate_ref) << std::endl;
  // std::cout << "fusion: " << args.get(fusion_ref) << std::endl;
  // std::cout << "ll_gpus: " << args.get(ll_gpus_ref) << std::endl;
//   CmdlineArgRef<int> ref ;
//   args.get(ref);
}