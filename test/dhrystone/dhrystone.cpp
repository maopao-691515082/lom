#include "../../include/lom.h"

struct Record
{
    typedef std::shared_ptr<Record> Ptr;

    Ptr ptr_comp;
    int discr = 0, enum_comp = 0, int_comp = 0;
    lom::Str str_comp;

    void Assign(const Ptr &other)
    {
        ptr_comp   = other->ptr_comp;
        discr      = other->discr;
        enum_comp  = other->enum_comp;
        int_comp   = other->int_comp;
        str_comp   = other->str_comp;
    }
};

static int64_t loops = 5000000;

enum : int
{
    Ident1 = 1,
    Ident2 = 2,
    Ident3 = 3,
    Ident4 = 4,
    Ident5 = 5,
};

static int int_glob;
static bool bool_glob;
static char char1_glob, char2_glob;

static lom::GoSlice<int> array1_glob(51);
static lom::GoSlice<lom::GoSlice<int>> array2_glob(51);

static Record::Ptr ptr_glb, ptr_glb_next;

static bool Func3(int enum_par_in)
{
    auto enum_loc = enum_par_in;
    return enum_loc == Ident3;
}

static int Func1(char char_par1, char char_par2)
{
    char char_loc1 = char_par1;
    char char_loc2 = char_loc1;
    return char_loc2 != char_par2 ? Ident1 : Ident2;
}

static bool Func2(const lom::Str &str_par_i1, const lom::Str &str_par_i2)
{
    int int_loc = 1;
    char char_loc = 0;
    while (int_loc <= 1)
    {
        if (Func1(str_par_i1.Get(int_loc), str_par_i2.Get(int_loc + 1)) == Ident1)
        {
            char_loc = 'A';
            ++ int_loc;
        }
    }
    if (char_loc >= 'W' && char_loc <= 'Z')
    {
        int_loc = 7;
    }
    if (char_loc == 'X')
    {
        return true;
    }
    if (str_par_i1 > str_par_i2)
    {
        int_loc += 7;
        return true;
    }
    return false;
}

static void Proc8(
    const lom::GoSlice<int> &array1_par, const lom::GoSlice<lom::GoSlice<int>> &array2_par,
    int int_par_i1, int int_par_i2)
{
    int int_loc = int_par_i1 + 5;
    array1_par.At(int_loc) = int_par_i2;
    array1_par.At(int_loc + 1) = array1_par.At(int_loc);
    array1_par.At(int_loc + 30) = int_loc;
    for (int int_index = int_loc; int_index < int_loc + 2; ++ int_index)
    {
        array2_par.At(int_loc).At(int_index) = int_loc;
    }
    ++ array2_par.At(int_loc).At(int_loc - 1);
    array2_par.At(int_loc + 20).At(int_loc) = array1_par.At(int_loc);
    int_glob = 5;
}

static int Proc7(int int_par_i1, int int_par_i2)
{
    int int_loc = int_par_i1 + 2;
    int int_par_out = int_par_i2 + int_loc;
    return int_par_out;
}

static int Proc6(int enum_par_in)
{
    int enum_par_out = enum_par_in;
    if (!Func3(enum_par_in))
    {
        enum_par_out = Ident4;
    }
    if (enum_par_in != Ident1)
    {
        enum_par_out = Ident1;
    }
    else if (enum_par_in == Ident2)
    {
        enum_par_out = int_glob > 100 ? Ident1 : Ident4;
    }
    else if (enum_par_in == Ident3)
    {
        enum_par_out = Ident2;
    }
    else if (enum_par_in == Ident4)
    {
    }
    else if (enum_par_in == Ident5)
    {
        enum_par_out = Ident3;
    }
    return enum_par_out;
}

static void Proc5()
{
    char1_glob = 'A';
    bool_glob = false;
}

static void Proc4()
{
    bool bool_loc = char1_glob == 'A';
    bool_loc = bool_loc || bool_glob;
    char2_glob = 'B';
}

static Record::Ptr Proc3(Record::Ptr ptr_par_out)
{
    if (ptr_glb)
    {
        ptr_par_out = ptr_glb->ptr_comp;
    }
    else
    {
        int_glob = 100;
    }
    ptr_glb->int_comp = Proc7(10, int_glob);
    return ptr_par_out;
}

static int Proc2(int int_par_io)
{
    int int_loc = int_par_io + 10;
    int enum_loc = 0;
    while (true)
    {
        if (char1_glob == 'A')
        {
            -- int_loc;
            int_par_io = int_loc - int_glob;
            enum_loc = Ident1;
        }
        if (enum_loc == Ident1)
        {
            break;
        }
    }
    return int_par_io;
}

static Record::Ptr Proc1(const Record::Ptr &ptr_par_in)
{
    Record::Ptr next_record = ptr_par_in->ptr_comp;
    next_record->Assign(ptr_glb);
    ptr_par_in->int_comp = 5;
    next_record->int_comp = ptr_par_in->int_comp;
    next_record->ptr_comp = ptr_par_in->ptr_comp;
    next_record->ptr_comp = Proc3(next_record->ptr_comp);
    if (next_record->discr == Ident1)
    {
        next_record->int_comp = 6;
        next_record->enum_comp = Proc6(ptr_par_in->enum_comp);
        next_record->ptr_comp = ptr_glb->ptr_comp;
        next_record->int_comp = Proc7(next_record->int_comp, 10);
    }
    else
    {
        ptr_par_in->Assign(next_record);
    }
    return ptr_par_in;
}

static void Proc0()
{
    ptr_glb_next = Record::Ptr(new Record);
    ptr_glb = Record::Ptr(new Record);

    ptr_glb->ptr_comp = ptr_glb_next;
    ptr_glb->discr = Ident1;
    ptr_glb->enum_comp = Ident3;
    ptr_glb->int_comp = 40;
    ptr_glb->str_comp = "DHRYSTONE PROGRAM, SOME STRING";

    lom::Str string1_loc = "DHRYSTONE PROGRAM, 1'ST STRING";

    array2_glob.At(8).At(7) = 10;

    for (int64_t i = 0; i < loops; ++ i)
    {
        Proc5();
        Proc4();
        int int_loc1 = 2;
        int int_loc2 = 3;
        int enum_loc = Ident2;
        lom::Str string2_loc = "DHRYSTONE PROGRAM, 2'ND STRING";
        bool_glob = !Func2(string1_loc, string2_loc);
        int int_loc3 = 0;
        while (int_loc1 < int_loc2)
        {
            int_loc3 = 5 * int_loc1 - int_loc2;
            int_loc3 = Proc7(int_loc1, int_loc2);
            ++ int_loc1;
        }
        Proc8(array1_glob, array2_glob, int_loc1, int_loc3);
        ptr_glb = Proc1(ptr_glb);
        for (char char_idx = 'A'; char_idx < char2_glob + 2; ++ char_idx)
        {
            if (enum_loc == Func1(char_idx, 'C'))
            {
                enum_loc = Proc6(Ident1);
            }
        }
        int_loc3 = int_loc2 * int_loc1;
        int_loc2 = int_loc3 / int_loc1;
        int_loc2 = 7 * (int_loc3 - int_loc2) - int_loc1;
        int_loc1 = Proc2(int_loc1);
    }
}

int main(int argc, char *argv[])
{
    for (ssize_t i = 0; i < 51; ++ i)
    {
        array2_glob.At(i) = lom::GoSlice<int>(51);
    }

    if (argc > 1)
    {
        if (!lom::Str(argv[1]).ParseInt64(loops))
        {
            fprintf(stderr, "invalid loop arg\n");
            exit(1);
        }
    }
    auto ts = lom::NowFloat();
    Proc0();
    auto tm = lom::NowFloat() - ts;
    printf("Time used: %f sec\n", tm);
    printf("This machine benchmarks at %f LomStones/second\n", static_cast<double>(loops) / tm);
}
