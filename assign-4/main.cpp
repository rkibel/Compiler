#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include "lir.cpp"
#include "json.hpp"

// for convenience
using json = nlohmann::json;


Terminal* get_terminal_instruction(json terminal){
    json::iterator term = terminal.begin();
    string key = term.key();
    json body = term.value();

    if(key == "Branch"){
        Branch* b = new Branch;
        b->ff = body["ff"];
        b->tt = body["tt"];
        json::iterator cond_type = body["cond"].begin();

        if(cond_type.key() == "Var"){
            Var* o = new Var;
            o->id = cond_type.value()["name"];
            b->guard = o;
        }
        else{
            Const* c = new Const;
            c->num = cond_type.value();
            b->guard = c;
        }
        return b;
    }
    else if(key == "CallDirect"){
        CallDirect* cd = new CallDirect;
        cd->lhs = body["lhs"]["name"];
        cd->callee = body["callee"];
        cd->next_bb = body["next_bb"];
        vector<Operand*> arguments;

        for(auto a : body["args"]){
            json::iterator arg_type = a.begin();

            if(arg_type.key() == "Var"){
                Var* o = new Var;
                o->id = arg_type.value()["name"];
                arguments.push_back(o);
            }
            else{
                Const* c = new Const;
                c->num = arg_type.value();
                arguments.push_back(c);
            }
        }
        cd->args = arguments;
        return cd;
    }
    else if(key == "CallIndirect"){
        CallIndirect* cid = new CallIndirect;
        cid->lhs = body["lhs"]["name"];
        cid->callee = body["callee"];
        cid->next_bb = body["next_bb"];
        vector<Operand*> arguments;

        for(auto a : body["args"]){
            json::iterator arg_type = a.begin();
            if(arg_type.key() == "Var"){
                Var* o = new Var;
                o->id = arg_type.value()["name"];
                arguments.push_back(o);
            }
            else{
                Const* c = new Const;
                c->num = arg_type.value();
                arguments.push_back(c);
            }
        }
        cid->args = arguments;
        return cid;
    }
    else if(key == "Jump"){
        Jump* j = new Jump;
        j->next_bb = body;
        return j;
    }
    else{
        Ret* r = new Ret;
        json::iterator return_type = body.begin();

        if(return_type.key() == "Var"){
            Var* o = new Var;
            o->id = return_type.value()["name"];
            r->op = o;
        }
        else{
            Const* c = new Const;
            c->num = return_type.value();
            r->op = c;
        }
        return r;
    }
}

LirInst* get_lir_instruction(json instruction){
    LirInst* li = new LirInst;
    json::iterator inst = instruction.begin();
    string key = inst.key();
    json body = inst.value();

    if(key == "Alloc"){

    }
    else if(key == "Arith"){

    }
    else if(key == "CallExt"){

    }
    else if(key == "Cmp"){

    }
    else if(key == "Copy"){

    }
    else if(key == "Gep"){

    }
    else if(key == "Gfp"){

    }
    else if(key == "Load"){

    }
    else{

    }

    return li;
}


// TODO :
Ptr handle_Ptr(json data){
    Ptr ptr;
    if(data == "Int"){  // base case
        ptr.ref = new Int;
        return ptr;
    }
    else {
        auto it = data.begin();
        if(it.key() == "Ptr"){
            Ptr* pointer = new Ptr;
            *pointer = handle_Ptr(data["Ptr"]);
            ptr.ref = pointer;
        }
        else if(it.key() == "Struct"){
            StructType* s = new StructType;
            s->name = data["Struct"];
            ptr.ref = s;

            //std::cout << data["Struct"] << std::endl;
        }


        // TODO : implement logic for initializing types for PARAMETERS and RETURN TYPE
        else if(it.key() == "Fn"){
            Fn* f = new Fn;
            for(auto param : data["Fn"]["param_ty"]){
                if(param == "Int"){
                    f->prms.push_back(new Int);
                }
                else{
                    f->prms.push_back(new Any);
                }
                // else{
                //     Type* temp = new Type;
                //     *temp = handle_Ptr(param);
                //     f->prms.push_back(temp);
                // }
            }
            if(data["Fn"]["ret_ty"] == "Int"){
                f->ret = new Int;
            }
            else{
                f->ret = new Any;
            }
            //std::cout << data << std::endl;
            ptr.ref = f;
        }
        
    }


    return ptr;
}

void copy_globals(Program* prog, json globals){
    map<VarId, Type*> globals_map;  

    for(auto d: globals){
        string name = d["name"];
        if(d["typ"] == "Int"){
            Int* temp = new Int;
            globals_map[name] = temp;
        }
        else {
            Ptr* temp = new Ptr;
            *temp = handle_Ptr(d["typ"]["Ptr"]);
            globals_map[name] = temp;
        }
    }
    prog->globals = globals_map;

}

// TODO :
void copy_structs(Program* prog, json structs){
    map<StructId, map<FieldId, Type*>> structs_map;  
    for (json::iterator it = structs.begin(); it != structs.end(); ++it) {
        string key = it.key();
        //std::cout << key << std::endl;
        for(auto field : it.value()){
            //std::cout << field["name"] << std::endl;

            // TODO : Get Type object of "field["typ"]"
            structs_map[key][field["name"]] = new Any;
        }
    }
    prog->structs = structs_map;
}

// TODO :
void copy_externs(Program* prog, json externs){
    map<FuncId, Type*> externs_map;  
    for (json::iterator it = externs.begin(); it != externs.end(); ++it) {
        string key = it.key();
        //std::cout << key << std::endl;

        // TODO : implement logic for initializing types for PARAMETERS and RETURN TYPE
        Fn* f = new Fn;
        for(auto param : it.value()["param_ty"]){
            if(param == "Int"){
                f->prms.push_back(new Int);
            }
            else{
                f->prms.push_back(new Any);
            }
            // else{
            //     Type* temp = new Type;
            //     *temp = handle_Ptr(param);
            //     f->prms.push_back(temp);
            // }
        }
        if(it.value()["ret_ty"] == "Int"){
            f->ret = new Int;
        }
        else{
            f->ret = new Any;
        }
        //std::cout << data << std::endl;
        externs_map[key] = f;
    }
    prog->externs = externs_map;
}


void copy_functions(Program* prog, json functions){
    map<FuncId, Function*> functions_map;
    for (json::iterator it = functions.begin(); it != functions.end(); ++it) {
        string key = it.key();
        Function* f = new Function;

        f->name = key;

        // TODO : implement logic for initializing types for PARAMETERS and RETURN TYPE
        for(auto param : it.value()["params"]){
            if(param == "Int"){
                f->params.push_back(std::pair(param["name"], new Int));
            }
            else{
                f->params.push_back(std::pair(param["name"], new Any));
            }
        }
        if(it.value()["ret_ty"] == "Int"){
            f->rettyp = new Int;
        }
        else{
            f->rettyp = new Any;
        }

        for(auto local : it.value()["locals"]){
            if(local["typ"] == "Int"){
                f->locals[local["name"]] = new Int;
            }
            else{
                f->locals[local["name"]] = new Any;
            }
        }

        for (json::iterator itr = it.value()["body"].begin(); itr != it.value()["body"].end(); ++itr) {
            // Loop for each BASIC BLOCK
            string key = itr.key();
            json instructions = itr.value()["insts"];
            
            BasicBlock* bb = new BasicBlock;
            bb->label = key;

            // Loop for each INSTRUCTION
            for(auto inst : instructions) {
                LirInst* lirInst = new LirInst;
                lirInst = get_lir_instruction(inst);
                bb->insts.push_back(lirInst);
            }
            
            // Get Termnial
            json terminal = itr.value()["term"]; 
            Terminal* termInst = new Terminal;
            termInst = get_terminal_instruction(terminal);
            bb->term = termInst;

            f->body[key] = bb;
        }

        functions_map[key] = f;
    }




    prog->functions = functions_map;
}   



Program initialize_prog(json data){
    Program prog;
    json globals = data["globals"];
    json structs = data["structs"];
    json externs = data["externs"];
    json functions = data["functions"];

    copy_globals(&prog, globals);
    copy_structs(&prog, structs);
    copy_externs(&prog, externs);
    copy_functions(&prog, functions);



    return prog;
}  


int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename.json>" << std::endl;
        return 1;
    }
    // Open the file
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }
    // Parse JSON data from the file
    json data;
    file >> data;
    // Close the file
    file.close();

    // std::cout << "Parsed JSON data:" << std::endl;
    // std::cout << data.dump(2) << std::endl; 

    Program prog = initialize_prog(data);

    
    
    


    std::cout << prog;
    
 
    
    return 0;
}