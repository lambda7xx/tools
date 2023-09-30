#include <variant>
#include <iostream>
#include <ostream>
#include <string>
#include <optional>
#include <unordered_map>

template <typename T>
struct CmdlineArgRef {
  std::string key;
  T value;
};

using AllowedArgTypes = std::variant<int, bool, float, std::string>; // we can extent this to support more types

//currently we only support "--xx" or "-x"
std::string parseKey(const std::string & arg) {
  std::cout<<"arg:"<<arg<<std::endl;
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
};

struct ArgsParser {
  std::unordered_map<std::string, Argument> mArguments;
};

template <typename T>
  CmdlineArgRef<T> add_argument(ArgsParser & parser, const std::string & key, const std::optional<T> & default_value,
                         const std::string &description, bool is_store_true = false) {
    std::string parse_key = parseKey(key);
    parser.mArguments[parse_key].description = description;
    if(default_value.has_value()) {  // Use has_value() to check if there's a value
        parser.mArguments[parse_key].value = std::to_string(default_value.value());  // Convert the value to string
        parser.mArguments[parse_key].default_value = true;
        parser.mArguments[key].is_store_true = is_store_true;
        //parser.mArguments[parse_key].is_store_true = true;
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

void parse_args(ArgsParser & mArgs, int argc, const char **argv) {
    int i  = 1;
    while (i  < argc) {
      std::cout<<"i:"<<i<<" and argv[i]:"<<argv[i]<<"and argc:"<<argc<<std::endl;
      std::string key = parseKey(argv[i]);
      if (key == "help" || key == "h") {
        exit(1);
      }
      // Check if the key is a store_true argument
        if(mArgs.mArguments.count(key) && mArgs.mArguments[key].is_store_true) {
            mArgs.mArguments[key].value = "true"; // Set to true if the flag is present
            i++;
            continue; // Skip the next iteration as there's no value associated with this flag
        }
      // Check if we have a value for this argument
        if (i + 1 < argc && (argv[i + 1][0] != '-' || (argv[i + 1][0] == '-' && argv[i + 1][1] == '-'))) {
            mArgs.mArguments[key].value = argv[i + 1];
            i += 2; // Increment to skip the value in the next iteration
        } else {
            i++; // Move to the next argument
        }

      //mArgs.mArguments[key].value = argv[i + 1];
    }
    //return mArgs;
  }

template <typename T>
T get(const ArgsParser & parser , const CmdlineArgRef<T> &ref)  {
    std::string key = ref.key;
    std::cout<<"1 T get key:"<<key<<std::endl;
    if(parser.mArguments.count(key)) {
      std::cout<<"2 T get key:"<<key<<"and parser.mArguments.at(key).is_store_true:"<<parser.mArguments.at(key).is_store_true <<std::endl;
        if(parser.mArguments.at(key).is_store_true) {
             return convert<T>(parser.mArguments.at(key).value.value_or("false"));
        } else if (parser.mArguments.at(key).default_value || parser.mArguments.at(key).value.has_value()) {
            return convert<T>(parser.mArguments.at(key).value.value());
        } 
    }
    throw std::runtime_error("invalid args: " + ref.key);
}


int main() {
    char const *test_argv[] = {"program_name",
                             "--batch-size",
                             "100",
                              "-ll:gpus",
                             "6",
                             "--fusion",
                             "false",
                             "--verbose"};
    
    ArgsParser args;
    auto batch_size_ref = add_argument(args, "--batch-size", std::optional<int>(32), "Size of each batch during training",false);
    auto ll_gpus_ref = add_argument<int>(args, "-ll:gpus", std::nullopt, "Number of GPUs to be used for training",false);
    auto verbose_ref = add_argument(args, "--verbose", std::optional<bool>(false), "Whether to print verbose logs", true);
    parse_args(args , 8, const_cast<const char **>(test_argv));

  // args.parse_args(9, const_cast<char **>(test_argv));
   std::cout<<"batch_size:"<<get(args, batch_size_ref)<<std::endl;
    std::cout<<"ll_gpus:"<<get(args, ll_gpus_ref)<<std::endl;
    bool is_verbose = get(args, verbose_ref);

    std::cout<<"verbose:"<<is_verbose<<std::endl;
  // std::cout << "batch_size: " << args.get(batch_size_ref) << std::endl;
  // std::cout << "learning_rate: " << args.get(learning_rate_ref) << std::endl;
  // std::cout << "fusion: " << args.get(fusion_ref) << std::endl;
  // std::cout << "ll_gpus: " << args.get(ll_gpus_ref) << std::endl;
//   CmdlineArgRef<int> ref ;
//   args.get(ref);
}