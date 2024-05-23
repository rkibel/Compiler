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
Type* get_type_object(json data);

Terminal* get_terminal_instruction(json terminal){
    //std::cout << "In get terminal\n";
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
        if(body["lhs"] == nullptr){
            cd->lhs = "_";
        }
        else{
            cd->lhs = body["lhs"]["name"];
        }
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
        if(body["lhs"] == nullptr){
            cid->lhs = "_";
        }
        else{
            cid->lhs = body["lhs"]["name"];
        }
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
    json::iterator inst = instruction.begin();
    string key = inst.key();
    json body = inst.value();

    if(key == "Alloc"){
        Alloc* a = new Alloc;
        a->lhs = body["lhs"]["name"];
        
        json::iterator num_info = body["num"].begin();

        if(num_info.key() == "Var"){
            Var* o = new Var;
            o->id = num_info.value()["name"];
            a->num = o;
        }
        else{
            Const* constant = new Const;
            constant->num = num_info.value();
            a->num = constant;
        }
        return a;
    }
    else if(key == "Arith"){
        Arith* a = new Arith;
        a->lhs = body["lhs"]["name"];

        if(body["aop"] == "Add"){
            Add* add = new Add;
            a->aop = add;
        }
        else if (body["aop"] == "Subtract"){
            Sub* sub = new Sub;
            a->aop = sub;
        }
        else if (body["aop"] == "Multiply"){
            Mul* mul = new Mul;
            a->aop = mul;
        }
        else{
            Div* div = new Div;
            a->aop = div;
        }

        json::iterator operand1_info = body["op1"].begin();

        if(operand1_info.key() == "Var"){
            Var* o = new Var;
            o->id = operand1_info.value()["name"];
            a->left = o;
        }
        else{
            Const* constant = new Const;
            constant->num = operand1_info.value();
            a->left = constant;
        }

        json::iterator operand2_info = body["op2"].begin();

        if(operand2_info.key() == "Var"){
            Var* o = new Var;
            o->id = operand2_info.value()["name"];
            a->right = o;
        }
        else{
            Const* constant = new Const;
            constant->num = operand2_info.value();
            a->right = constant;
        }

        return a;
    }
    else if(key == "CallExt"){
        //std::cout << "In callext\n";
        CallExt* cext = new CallExt;
        cext->callee = body["ext_callee"];
        //std::cout << "After callee\n";
        if(body["lhs"] == nullptr){
            cext->lhs = "_";
        }
        else{
            cext->lhs = body["lhs"]["name"];
        }
        //std::cout << "Starting args loop\n";
        vector<Operand*> arguments;
        for(auto a : body["args"]){
            json::iterator arg_type = a.begin();
            if(arg_type.key() == "Var"){
                //std::cout << "In var\n";
                Var* o = new Var;
                o->id = arg_type.value()["name"];
                arguments.push_back(o);
            }
            else{
                //std::cout << "In const\n";
                Const* c = new Const;
                c->num = arg_type.value();
                arguments.push_back(c);
            }
        }
        //std::cout << "After args loop\n";
        cext->args = arguments;
        return cext;
    }
    else if(key == "Cmp"){
        Cmp* cmp = new Cmp;
        cmp->lhs = body["lhs"]["name"];

        if(body["rop"] == "Eq"){
            Equal* eq = new Equal;
            cmp->cop = eq;
        }
        else if (body["rop"] == "Neq"){
            NotEq* neq = new NotEq;
            cmp->cop = neq;
        }
        else if (body["rop"] == "Less"){
            Lt* lt = new Lt;
            cmp->cop = lt;
        }
        else if (body["rop"] == "LessEq"){
            Lte* lte = new Lte;
            cmp->cop = lte;
        }
        else if (body["rop"] == "Greater"){
            Gt* gt = new Gt;
            cmp->cop = gt;
        }
        else{
            Gte* gte = new Gte;
            cmp->cop = gte;
        }
        
        json::iterator operand1_info = body["op1"].begin();

        if(operand1_info.key() == "Var"){
            Var* o = new Var;
            o->id = operand1_info.value()["name"];
            cmp->left = o;
        }
        else{
            Const* constant = new Const;
            constant->num = operand1_info.value();
            cmp->left = constant;
        }

        json::iterator operand2_info = body["op2"].begin();

        if(operand2_info.key() == "Var"){
            Var* o = new Var;
            o->id = operand2_info.value()["name"];
            cmp->right = o;
        }
        else{
            Const* constant = new Const;
            constant->num = operand2_info.value();
            cmp->right = constant;
        }
        return cmp;
    }
    else if(key == "Copy"){
        Copy* c = new Copy;
        c->lhs = body["lhs"]["name"];
        json::iterator operand_info = body["op"].begin();

        if(operand_info.key() == "Var"){
            Var* o = new Var;
            o->id = operand_info.value()["name"];
            c->op = o;
        }
        else{
            Const* constant = new Const;
            constant->num = operand_info.value();
            c->op = constant;
        }

        return c;
    }
    else if(key == "Gep"){
        Gep* g = new Gep;
        g->lhs = body["lhs"]["name"];
        g->src = body["src"]["name"];

        json::iterator idx_info = body["idx"].begin();

        if(idx_info.key() == "Var"){
            Var* o = new Var;
            o->id = idx_info.value()["name"];
            g->idx = o;
        }
        else{
            Const* constant = new Const;
            constant->num = idx_info.value();
            g->idx = constant;
        }
        return g;
    }
    else if(key == "Gfp"){
        Gfp* g = new Gfp;
        g->lhs = body["lhs"]["name"];
        g->src = body["src"]["name"];
        g->field = body["field"]["name"];
        return g;
    }
    else if(key == "Load"){
        Load* l = new Load;
        l->lhs = body["lhs"]["name"];
        l->src = body["src"]["name"];
        return l;
    }
    else{
        Store* s = new Store;
        s->dst = body["dst"]["name"];

        json::iterator op_info = body["op"].begin();

        if(op_info.key() == "Var"){
            Var* o = new Var;
            o->id = op_info.value()["name"];
            s->op = o;
        }
        else{
            Const* constant = new Const;
            constant->num = op_info.value();
            s->op = constant;
        }
        return s;
    }
}

Ptr* handle_Ptr(json data){
    Ptr* ptr = new Ptr;

    if(data == nullptr || data.empty()){
        ptr->ref = new Any;
        return ptr;
    }
    else if(data == "Int"){
        ptr->ref = new Int;
        return ptr;
    }
    else{
        auto it = data.begin();
        if(it.key() == "Ptr"){
            Ptr* pointer = new Ptr;
            pointer = handle_Ptr(it.value());
            ptr->ref = pointer;
            return ptr;
        }
        else if(it.key() == "Struct"){
            StructType* s = new StructType;
            s->name = it.value();
            ptr->ref = s;
            return ptr;
        }
        else{
            Fn* f = new Fn;
            for(auto param : it.value()["param_ty"]){
                Type* temp = new Type;
                temp = get_type_object(param);
                f->prms.push_back(temp);
            }
            Type* temp = new Type;
            temp = get_type_object(it.value()["ret_ty"]);
            f->ret = temp;
            ptr->ref = f;
            return ptr;
        }
    }
}
Type* get_type_object(json data){
    if(data == nullptr || data.empty()){
        return new Any;
    }
    else if(data == "Int"){
        Int* i = new Int;
        return i;
    }
    else {
        Ptr* temp = new Ptr;
        temp = handle_Ptr(data["Ptr"]);
        return temp;
    }
}

void copy_globals(Program* prog, json globals){
    map<VarId, Type*> globals_map;  

    for(auto d: globals){
        string name = d["name"];
        Type* temp = new Type;

        temp = get_type_object(d["typ"]);

        // if(d["typ"] == "Int"){
        //     Int* temp = new Int;
        //     globals_map[name] = temp;
        // }
        // else {
        //     Ptr* temp = new Ptr;
        //     *temp = handle_Ptr(d["typ"]["Ptr"]);
        //     globals_map[name] = temp;
        // }

        globals_map[name] = temp;
    }
    prog->globals = globals_map;

}
void copy_structs(Program* prog, json structs){
    map<StructId, map<FieldId, Type*>> structs_map;  
    for (json::iterator it = structs.begin(); it != structs.end(); ++it) {
        string key = it.key();
        for(auto field : it.value()){
            Type* t = new Type;
            t = get_type_object(field["typ"]);
            structs_map[key][field["name"]] = t;
        }
    }
    prog->structs = structs_map;
}
void copy_externs(Program* prog, json externs){
    map<FuncId, Type*> externs_map;  
    for (json::iterator it = externs.begin(); it != externs.end(); ++it) {
        string key = it.key();
        Fn* f = new Fn;
        for(auto param : it.value()["Fn"]["param_ty"]){
            Type* temp = new Type;
            temp = get_type_object(param);
            f->prms.push_back(temp);
        }
        Type* temp = new Type;
        temp = get_type_object(it.value()["Fn"]["ret_ty"]);
        f->ret = temp;
        externs_map[key] = f;
    }
    prog->externs = externs_map;
}
void copy_functions(Program* prog, json functions){
    map<FuncId, Function*> functions_map;
    for (json::iterator it = functions.begin(); it != functions.end(); ++it) {
        string key = it.key();
        //std::cout << "On function " + key + "\n\n";
        Function* f = new Function;
        f->name = key;
        for(auto param : it.value()["params"]){
            Type* temp = new Type;
            temp = get_type_object(param["typ"]);
            f->params.push_back(std::pair(param["name"], temp));
        }
        Type* temp = new Type;
        temp = get_type_object(it.value()["ret_ty"]);
        f->rettyp = temp;

        for(auto local : it.value()["locals"]){
            Type* temp = new Type;
            temp = get_type_object(local["typ"]);
            f->locals[local["name"]] = temp;
        }

        for (json::iterator itr = it.value()["body"].begin(); itr != it.value()["body"].end(); ++itr) {
            string key = itr.key();
            json instructions = itr.value()["insts"];
            BasicBlock* bb = new BasicBlock;
            bb->label = key;
            //std::cout << "Before instruction loop\n\n";
            for(auto inst : instructions) {
                LirInst* lirInst = new LirInst;
                lirInst = get_lir_instruction(inst);
                //std::cout << "After get_lir\n";
                bb->insts.push_back(lirInst);
            }
            //std::cout << "After instruction loop\n\n";
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
    // std::cout << prog;
    std::cout << prog.cg();
    
    return 0;
}