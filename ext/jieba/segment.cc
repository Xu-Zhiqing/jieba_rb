#include "segment.h"
#include <ruby/encoding.h>
#include <MPSegment.hpp>
#include <HMMSegment.hpp>
#include <MixSegment.hpp>

static rb_encoding* u8_enc;

struct SegWrapper{
    cppjieba::MixSegment *mixp;
    cppjieba::HMMSegment *hmmp;
    cppjieba::MPSegment *mpsp;
    SegWrapper(): mixp(nullptr), hmmp(nullptr), mpsp(nullptr) {}
};

static void seg_free(void *p){
    auto seg = reinterpret_cast<SegWrapper *>(p);
    if (seg->mixp)
        delete seg->mixp;
    else if (seg->hmmp)
        delete seg->hmmp;
    else
        delete seg->mpsp;
    delete seg;
}

static VALUE allocate(VALUE klass)
{
    SegWrapper* seg_wrapper = new SegWrapper();
    return Data_Wrap_Struct(klass, NULL, seg_free, seg_wrapper);
}

static void seg_init(VALUE self,
                     VALUE type_rb_sym,
                     VALUE jieba_dict_rbs,
                     VALUE hmm_dict_rbs,
                     VALUE user_dict_rbs)
{
    SegWrapper* seg_wrapper;
    Data_Get_Struct(self, SegWrapper, seg_wrapper);

    Check_Type(jieba_dict_rbs, T_STRING);
    Check_Type(hmm_dict_rbs, T_STRING);
    Check_Type(user_dict_rbs, T_STRING);

    std::string jieba_dict = StringValueCStr(jieba_dict_rbs);
    std::string hmm_dict = StringValueCStr(hmm_dict_rbs);
    std::string user_dict = StringValueCStr(user_dict_rbs);

    ID type = SYM2ID(type_rb_sym);
    if ( type == rb_intern("mix") )
    {
        seg_wrapper->mixp = new cppjieba::MixSegment(jieba_dict, hmm_dict, user_dict);
    }
    else if ( type == rb_intern("hmm") )
    {
        seg_wrapper->hmmp = new cppjieba::HMMSegment(hmm_dict);
    }
    else if ( type == rb_intern("mp"))
    {
        seg_wrapper->mpsp = new cppjieba::MPSegment(jieba_dict);
    }
}

static VALUE seg_cut(VALUE self, VALUE text_rbs)
{
    Check_Type(text_rbs, T_STRING);
    std::string text = StringValueCStr(text_rbs);

    SegWrapper* seg_wrapper;
    Data_Get_Struct(self, SegWrapper, seg_wrapper);

    std::vector<std::string> words;

    if (seg_wrapper->mixp) {
        seg_wrapper->mixp->Cut(text, words);
    }
    else if (seg_wrapper->hmmp) {
        seg_wrapper->hmmp->Cut(text, words);
    }
    else {
        seg_wrapper->mpsp->Cut(text, words);
    }

    volatile VALUE arr = rb_ary_new();
    for (std::vector<std::string>::const_iterator j = words.begin(); j != words.end(); j++)
    {

        rb_ary_push(arr, rb_enc_str_new((*j).c_str(), (*j).length(), u8_enc));

    }
    return arr;
}

#define DEF(k,n,f,c) rb_define_method(k,n,RUBY_METHOD_FUNC(f),c)

extern "C" {
    void Init_segment()
    {
        VALUE cSegment = rb_define_class_under(mJieba, "Segment", rb_cObject);
        u8_enc = rb_utf8_encoding();
        rb_define_alloc_func(cSegment, allocate);
        DEF(cSegment, "_init",seg_init,4);
        DEF(cSegment, "cut",seg_cut,1);
    }

}
