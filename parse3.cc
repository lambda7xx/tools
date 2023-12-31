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
    bool is_optional = false;
};

struct ArgsParser {
  std::unordered_map<std::string, Argument> mArguments;
  int num_required_args = 0;
  int pass_required_args = 0; 
};

//default_value is std::nullopt 
// template <typename T>
//   CmdlineArgRef<T> add_required_argument(ArgsParser & parser, const std::string & key, const std::optional<T> & default_value,
//                          const std::string &description, bool is_store_true = false) {
//     std::string parse_key = parseKey(key);
//     parser.requeiredArguments[parse_key].description = description;
//     if(default_value.has_value()) {  // Use has_value() to check if there's a value
//         parser.requeiredArguments[parse_key].value = std::to_string(default_value.value());  // Convert the value to string
//         parser.requeiredArguments[parse_key].default_value = true;
//         parser.requeiredArguments[parse_key].is_store_true = is_store_true;
//         return CmdlineArgRef<T>{parse_key, default_value.value()};
//     } 
//     return CmdlineArgRef<T>{parse_key, T{}};
//   }


// template <typename T>
//   CmdlineArgRef<T> add_optional_argument(ArgsParser & parser, const std::string & key, const std::optional<T> & default_value,
//                          const std::string &description, bool is_store_true = false) {
//     std::string parse_key = parseKey(key);
//     parser.optionalArguments[parse_key].description = description;
//     parser.optionalArguments[parse_key].is_store_true = is_store_true;
//     parser.num_optional_args++;
//     return CmdlineArgRef<T>{parse_key, T{}};
//   }

//default_value is std::nullopt 
template <typename T>
  CmdlineArgRef<T> add_required_argument(ArgsParser & parser, const std::string & key, const std::optional<T> & default_value,
                         const std::string &description, bool is_store_true = false) {
    std::string parse_key = parseKey(key);
    parser.mArguments[parse_key].description = description;
    parser.mArguments[parse_key].is_store_true = is_store_true;
    parser.num_required_args++;
    std::cout<<"add_required_argument, parser.num_required_args:"<<parser.num_required_args<<std::endl;
    parser.mArguments[parse_key].is_optional = false;
    return CmdlineArgRef<T>{parse_key, T{}};
  }


template <typename T>
  CmdlineArgRef<T> add_optional_argument(ArgsParser & parser, const std::string & key, const std::optional<T> & default_value,
                         const std::string &description, bool is_store_true = false) {
    std::string parse_key = parseKey(key);
    parser.mArguments[parse_key].description = description;
    if(default_value.has_value()) {  // Use has_value() to check if there's a value
        parser.mArguments[parse_key].value = std::to_string(default_value.value());  // Convert the value to string
        parser.mArguments[parse_key].default_value = true;
        parser.mArguments[parse_key].is_store_true = is_store_true;
        parser.mArguments[parse_key].is_optional = true;
        return CmdlineArgRef<T>{parse_key, default_value.value()};
    } 
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
    std::vector<std::string> required_args_passed;
   for(const auto & [key, arg] : mArgs.mArguments) {
    result.mArguments[key] = arg;
  }
  result.num_required_args = mArgs.num_required_args;
  std::cout<<"parse_args, mArgs.num_required_args:"<<mArgs.num_required_args<<" and  result.num_required_args:"<<result.num_required_args<<std::endl;

    while (i  < argc) {
        std::string key = parseKey(argv[i]);
        if (key == "help" || key == "h") {
            exit(1);
        }

        if(mArgs.mArguments.count(key) && mArgs.mArguments.at(key).is_store_true) {
            result.mArguments[key].value = "true";
            result.mArguments[key].is_store_true = true;
            result.mArguments[key].is_store_passed = true;
            i++;
            continue; 
        }

        if (i + 1 < argc && argv[i + 1][0] != '-') {
            if(result.mArguments.count(key)) {
                if(result.mArguments.at(key).is_optional) {
                  result.mArguments[key].value = argv[i + 1];
                } else {
                  //required args
                  result.mArguments[key].value = argv[i + 1];
                  result.pass_required_args++;
                  required_args_passed.push_back(key);
                }
            }else {
                throw std::runtime_error("invalid args: " + key + " does not exist") ;
            }
            i += 2; 
        } else {
            if (result.mArguments.count(key) && !result.mArguments.at(key).is_store_true) {
                throw std::runtime_error("required args: " + key + " needs a value");
            }
            i++; 
        }
    }
    std::cout<<"result.pass_required_args:"<<result.pass_required_args<<" and  result.pass_required_args:"<<result.pass_required_args<<std::endl;
    if(result.num_required_args != result.pass_required_args) {
        std::vector<std::string> missing_args;
        for(const auto & [key, arg] : mArgs.mArguments) {
            if(!arg.is_optional) {//required args
                if(std::find(required_args_passed.begin(), required_args_passed.end(), key) == required_args_passed.end()) {
                    missing_args.push_back(key);
                }
            }
        }
        //std::string missing_args_str = "";
        for(const auto & arg : missing_args) {
           // missing_args_str +=  arg + "  " ;
            std::cout<<"missing_args:"<<arg<<std::endl;
        }
        throw std::runtime_error("some required args are not passed");
    }

    return result;
  }

template <typename T> 
T get(const ArgsParser & parser , const CmdlineArgRef<T> &ref)  {
    std::string key = ref.key;
    if(parser.mArguments.count(key)) {
      if(parser.mArguments.at(key).is_store_true) {
        if(parser.mArguments.at(key).is_store_passed) {
          return true;
        } else {
          return false;
        }
      } else {
        return convert<T>(parser.mArguments.at(key).value.value());
        // if(parser.mArguments.at(key).is_optional) {
        //   return convert<T>(parser.mArguments.at(key).value.value());
        // } else {
        //   return convert<T>(parser.mArguments.at(key).value.value());
        // }
      }
    }
    throw std::runtime_error("invalid args: " + ref.key);
}

/** normal test
int main() {


    char const *test_argv[] = {"program_name",
                             "--batch-size",
                             "100",
                              "-ll:gpus",
                             "6",
                             "--fusion",
                             "false"};
    
    ArgsParser args;
    auto batch_size_ref = add_optional_argument(args, "--batch-size", std::optional<int>(32), "Size of each batch during training");
    auto learning_rate_ref = add_optional_argument(args, "--learning-rate", std::optional<float>(0.001), "Learning rate for the optimizer");
    auto fusion_ref = add_required_argument(args, "--fusion", std::optional<bool>(true), "Whether to use fusion or not");
    auto ll_gpus_ref = add_required_argument<int>(args, "-ll:gpus", std::nullopt, "Number of GPUs to be used for training");
    constexpr size_t test_argv_length = sizeof(test_argv) / sizeof(test_argv[0]);

    ArgsParser result = parse_args(args , test_argv_length, const_cast<const char **>(test_argv));
    std::cout<<"batch_size:"<<get(result, batch_size_ref)<<std::endl;
    std::cout<<"learning_rate:"<<get(result, learning_rate_ref)<<std::endl;
    std::cout<<"ll_gpus:"<<get(result, ll_gpus_ref)<<std::endl;
}
**/

/*test with store_true， pass the verbose via command line or not
int main() {
    
    // char const *test_argv[] = {"program_name",
    //                          "--batch-size",
    //                          "100",
    //                           "-ll:gpus",
    //                          "6",
    //                          "--fusion",
    //                          "false",
    //                          "--verbose"};
        char const *test_argv[] = {"program_name",
                             "--batch-size",
                             "100",
                              "-ll:gpus",
                             "6",
                             "--fusion",
                             "false"};
    ArgsParser args;
    auto batch_size_ref = add_optional_argument(args, "--batch-size", std::optional<int>(32), "Size of each batch during training");
    auto learning_rate_ref = add_optional_argument(args, "--learning-rate", std::optional<float>(0.001), "Learning rate for the optimizer");
    auto fusion_ref = add_required_argument(args, "--fusion", std::optional<bool>(true), "Whether to use fusion or not");
    auto ll_gpus_ref = add_required_argument<int>(args, "-ll:gpus", std::nullopt, "Number of GPUs to be used for training");
    auto verbose_ref = add_required_argument(args, "--verbose", std::optional<bool>(false), "Whether to print verbose logs",true);

    constexpr size_t test_argv_length = sizeof(test_argv) / sizeof(test_argv[0]);

    ArgsParser result = parse_args(args , test_argv_length, const_cast<const char **>(test_argv));
    std::cout<<"batch_size:"<<get(result, batch_size_ref)<<std::endl;
    std::cout<<"learning_rate:"<<get(result, learning_rate_ref)<<std::endl;
    std::cout<<"ll_gpus:"<<get(result, ll_gpus_ref)<<std::endl;
    std::cout<<"verbose:"<<get(result, verbose_ref)<<std::endl;
}
*/

/*test batch_size via command , it will throw exception, because we only support --batch-size or -batch-size via command line
int main() {
    
    // char const *test_argv[] = {"program_name",
    //                          "--batch-size",
    //                          "100",
    //                           "-ll:gpus",
    //                          "6",
    //                          "--fusion",
    //                          "false",
    //                          "--verbose"};
        char const *test_argv[] = {"program_name",
                             "batch-size",
                             "100",
                              "-ll:gpus",
                             "6",
                             "--fusion",
                             "false"};
    ArgsParser args;
    auto batch_size_ref = add_optional_argument(args, "--batch-size", std::optional<int>(32), "Size of each batch during training");
    auto learning_rate_ref = add_optional_argument(args, "--learning-rate", std::optional<float>(0.001), "Learning rate for the optimizer");
    auto fusion_ref = add_required_argument(args, "--fusion", std::optional<bool>(true), "Whether to use fusion or not");
    auto ll_gpus_ref = add_required_argument<int>(args, "-ll:gpus", std::nullopt, "Number of GPUs to be used for training");
    auto verbose_ref = add_required_argument(args, "--verbose", std::optional<bool>(false), "Whether to print verbose logs",true);

    constexpr size_t test_argv_length = sizeof(test_argv) / sizeof(test_argv[0]);

    ArgsParser result = parse_args(args , test_argv_length, const_cast<const char **>(test_argv));
    std::cout<<"batch_size:"<<get(result, batch_size_ref)<<std::endl;
    std::cout<<"learning_rate:"<<get(result, learning_rate_ref)<<std::endl;
    std::cout<<"ll_gpus:"<<get(result, ll_gpus_ref)<<std::endl;
    std::cout<<"verbose:"<<get(result, verbose_ref)<<std::endl;
}*/

// //test the invalid ref 
// int main() {
    
//     // char const *test_argv[] = {"program_name",
//     //                          "--batch-size",
//     //                          "100",
//     //                           "-ll:gpus",
//     //                          "6",
//     //                          "--fusion",
//     //                          "false",
//     //                          "--verbose"};
//     /*char const *test_argv[] = {"program_name",
//                              "--batch-size",
//                              "100",
//                               "-ll:gpus",
//                              "6",
//                              "--fusion",
//                              "false"};
//     ArgsParser args;
//     auto batch_size_ref = add_optional_argument(args, "--batch-size", std::optional<int>(32), "Size of each batch during training");
//     auto learning_rate_ref = add_optional_argument(args, "--learning-rate", std::optional<float>(0.001), "Learning rate for the optimizer");
//     auto fusion_ref = add_required_argument(args, "--fusion", std::optional<bool>(true), "Whether to use fusion or not");
//     auto ll_gpus_ref = add_required_argument<int>(args, "-ll:gpus", std::nullopt, "Number of GPUs to be used for training");
//     auto verbose_ref = add_required_argument(args, "--verbose", std::optional<bool>(false), "Whether to print verbose logs",true);

//     constexpr size_t test_argv_length = sizeof(test_argv) / sizeof(test_argv[0]);

//     ArgsParser result = parse_args(args , test_argv_length, const_cast<const char **>(test_argv));
//     std::cout<<"batch_size:"<<get(result, batch_size_ref)<<std::endl;
//     std::cout<<"learning_rate:"<<get(result, learning_rate_ref)<<std::endl;
//     std::cout<<"ll_gpus:"<<get(result, ll_gpus_ref)<<std::endl;
//     std::cout<<"verbose:"<<get(result, verbose_ref)<<std::endl;*/
//     CmdlineArgRef<int> invalid_ref{"invalid", {}};
//     char const *test_argv[] = {"program_name"};

//     ArgsParser args;
//     parse_args(args, 1, const_cast<const char **>(test_argv));
//     get(args, invalid_ref); // throw exception  because it's invalid ref
// }

/*test with store_true， pass the verbose via command line or not
int main() {
    
    // char const *test_argv[] = {"program_name",
    //                          "--batch-size",
    //                          "100",
    //                           "-ll:gpus",
    //                          "6",
    //                          "--fusion",
    //                          "false",
    //                          "--verbose"};
        char const *test_argv[] = {"program_name",
                             "--batch-size",
                             "100",
                              "-ll:gpus",
                             "6",
                             "--fusion",
                             "false"};
    ArgsParser args;
    auto batch_size_ref = add_optional_argument(args, "--batch-size", std::optional<int>(32), "Size of each batch during training");
    auto learning_rate_ref = add_optional_argument(args, "--learning-rate", std::optional<float>(0.001), "Learning rate for the optimizer");
    auto fusion_ref = add_required_argument(args, "--fusion", std::optional<bool>(true), "Whether to use fusion or not");
    auto ll_gpus_ref = add_required_argument<int>(args, "-ll:gpus", std::nullopt, "Number of GPUs to be used for training");
    auto verbose_ref = add_required_argument(args, "--verbose", std::optional<bool>(false), "Whether to print verbose logs",true);

    constexpr size_t test_argv_length = sizeof(test_argv) / sizeof(test_argv[0]);

    ArgsParser result = parse_args(args , test_argv_length, const_cast<const char **>(test_argv));
    std::cout<<"batch_size:"<<get(result, batch_size_ref)<<std::endl;
    std::cout<<"learning_rate:"<<get(result, learning_rate_ref)<<std::endl;
    std::cout<<"ll_gpus:"<<get(result, ll_gpus_ref)<<std::endl;
    std::cout<<"verbose:"<<get(result, verbose_ref)<<std::endl;
}
*/

//the verbose is not passed via command line, so it will throw exception
/*int main() {

    char const *test_argv[] = {
      "program_name", "--batch-size", "100"};
      ArgsParser args;
  auto batch_size_ref = add_optional_argument(
      args, "--batch-size", std::optional<int>(32), "batch size for training");
  auto ll_gpus_ref = add_required_argument<int>(
      args,
      "-ll:gpus",
      std::nullopt,
      "Number of GPUs to be used for training"); // support non-default value
  constexpr size_t test_argv_length = sizeof(test_argv) / sizeof(test_argv[0]);
  ArgsParser result = parse_args(
      args,
      test_argv_length,
      const_cast<const char **>(test_argv)); // throw exception because we don't pass
                                        // -ll:gpus via command
 // std::cout << "batch_size:" << get(result, batch_size_ref) << std::endl;
// get(result, ll_gpus_ref);
} */

// ./a.out --args1 4  --arg2 4 -args3  throw exceptions
/*int main( ) {
    char const *test_argv[] = {"program_name",
                               "--batch-size",
                               "100",
                               "--learning-rate",
                               "0.03",
                               "--epoch"};
    ArgsParser args;
    auto batch_size_ref =
        add_optional_argument(args,
                              "--batch-size",
                              std::optional<int>(32),
                              "Size of each batch during training");
    auto learning_rate_ref =
        add_optional_argument(args,
                              "--learning-rate",
                              std::optional<float>(0.001),
                              "Learning rate for the optimizer");
    auto epoch_ref = add_optional_argument(args,
                                           "--epoch",
                                           std::optional<int>(1),
                                           "Number of epochs for training");
    constexpr size_t test_argv_length =
        sizeof(test_argv) / sizeof(test_argv[0]);
    parse_args(
        args, test_argv_length, const_cast<char const **>(test_argv));
}*/

//./a.out --args 4  --arg2 -args3 4
int main() {
      char const *test_argv[] = {
        "program_name",
        "--batch-size",
        "100",
        "--epoch",
        "--learning-rate",
        "0.03",
    };
    ArgsParser args;
    auto batch_size_ref =
        add_optional_argument(args,
                              "--batch-size",
                              std::optional<int>(32),
                              "Size of each batch during training");
    auto learning_rate_ref =
        add_optional_argument(args,
                              "--learning-rate",
                              std::optional<float>(0.001),
                              "Learning rate for the optimizer");
    auto epoch_ref = add_optional_argument(args,
                                           "--epoch",
                                           std::optional<int>(1),
                                           "Number of epochs for training");
    constexpr size_t test_argv_length =
        sizeof(test_argv) / sizeof(test_argv[0]);
    parse_args(
        args, test_argv_length, const_cast<char const **>(test_argv));
}