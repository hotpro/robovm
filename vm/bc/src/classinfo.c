#include <string.h>
#include "packed.h"
#include "classinfo.h"

#define FI_ACCESS_MASK 0x3
#define FI_PUBLIC 0x1
#define FI_PRIVATE 0x2
#define FI_PROTECTED 0x3
#define FI_STATIC 0x4
#define FI_FINAL 0x8
#define FI_VOLATILE 0x10
#define FI_TRANSIENT 0x20
#define FI_SYNTHETIC 0x40
#define FI_ENUM 0x80
#define FI_ATTRIBUTES 0x100

#define MI_ACCESS_MASK 0x3
#define MI_PUBLIC 0x1
#define MI_PRIVATE 0x2
#define MI_PROTECTED 0x3
#define MI_STATIC 0x4
#define MI_FINAL 0x8
#define MI_SYNCHRONIZED 0x10
#define MI_BRIDGE 0x20
#define MI_VARARGS 0x40
#define MI_NATIVE 0x80
#define MI_ABSTRACT 0x100
#define MI_STRICT 0x200
#define MI_SYNTHETIC 0x400
#define MI_ATTRIBUTES 0x800
#define MI_BRO_BRIDGE 0x1000
#define MI_BRO_CALLBACK 0x2000
#define MI_COMPACT_DESC 0x4000

#define DESC_B 1
#define DESC_C 2
#define DESC_D 3
#define DESC_F 4
#define DESC_I 5
#define DESC_J 6
#define DESC_S 7
#define DESC_Z 8
#define DESC_V 9

// Descriptor strings used for compact descriptors. It is important that
// we use the same symbols as the compiled lookup functions use. Otherwise
// proxy.c will not be able to determine the method called on proxy classes.
extern char str__28_29B_00;
extern char str__28_29C_00;
extern char str__28_29D_00;
extern char str__28_29F_00;
extern char str__28_29I_00;
extern char str__28_29J_00;
extern char str__28_29S_00;
extern char str__28_29Z_00;
extern char str__28_29V_00;
static const char* B = &str__28_29B_00;
static const char* C = &str__28_29C_00;
static const char* D = &str__28_29D_00;
static const char* F = &str__28_29F_00;
static const char* I = &str__28_29I_00;
static const char* J = &str__28_29J_00;
static const char* S = &str__28_29S_00;
static const char* Z = &str__28_29Z_00;
static const char* V = &str__28_29V_00;

static jboolean parseFieldInfo(Env* env, ClassInfoHeader* header, void** p, ParseClassInfoCallbacks* callbacks, void* data) {
    jint flags = readShort(p);
    char* name = readString(p);
    jint access = 0;
    if ((flags & FI_ACCESS_MASK) == FI_PUBLIC) access |= ACC_PUBLIC;
    if ((flags & FI_ACCESS_MASK) == FI_PRIVATE) access |= ACC_PRIVATE;
    if ((flags & FI_ACCESS_MASK) == FI_PROTECTED) access |= ACC_PROTECTED;
    if (flags & FI_STATIC) access |= ACC_STATIC;
    if (flags & FI_FINAL) access |= ACC_FINAL;
    if (flags & FI_VOLATILE) access |= ACC_VOLATILE;
    if (flags & FI_TRANSIENT) access |= ACC_TRANSIENT;
    if (flags & FI_SYNTHETIC) access |= ACC_SYNTHETIC;
    if (flags & FI_ENUM) access |= ACC_ENUM;

    const char* desc = NULL;
    if ((flags >> 12) != 0) {
        switch ((flags >> 12) & 0xf) {
        case DESC_B: desc = "B"; break;
        case DESC_C: desc = "C"; break;
        case DESC_D: desc = "D"; break;
        case DESC_F: desc = "F"; break;
        case DESC_I: desc = "I"; break;
        case DESC_J: desc = "J"; break;
        case DESC_S: desc = "S"; break;
        case DESC_Z: desc = "Z"; break;
        }
    } else {
        desc = readString(p);
    }

    jint offset = readInt(p);

    void* attributes = NULL;
    if (flags & FI_ATTRIBUTES) attributes = readPtr(p);

    if (!callbacks->fieldCallback) return TRUE;
    return callbacks->fieldCallback(env, header, name, desc, access, offset, attributes, data);
}

static jboolean parseMethodInfo(Env* env, ClassInfoHeader* header, void** p, ParseClassInfoCallbacks* callbacks, void* data) {
    jint flags = readShort(p);
    char* name = readString(p);
    jint access = 0;
    if ((flags & MI_ACCESS_MASK) == MI_PUBLIC) access |= ACC_PUBLIC;
    if ((flags & MI_ACCESS_MASK) == MI_PRIVATE) access |= ACC_PRIVATE;
    if ((flags & MI_ACCESS_MASK) == MI_PROTECTED) access |= ACC_PROTECTED;
    if (flags & MI_STATIC) access |= ACC_STATIC;
    if (flags & MI_FINAL) access |= ACC_FINAL;
    if (flags & MI_SYNCHRONIZED) access |= ACC_SYNCHRONIZED;
    if (flags & MI_BRIDGE) access |= ACC_BRIDGE;
    if (flags & MI_VARARGS) access |= ACC_VARARGS;
    if (flags & MI_NATIVE) access |= ACC_NATIVE;
    if (flags & MI_ABSTRACT) access |= ACC_ABSTRACT;
    if (flags & MI_STRICT) access |= ACC_STRICT;

    const char* desc = NULL;
    if (flags & MI_COMPACT_DESC) {
        switch (readByte(p)) {
        case DESC_B: desc = B; break;
        case DESC_C: desc = C; break;
        case DESC_D: desc = D; break;
        case DESC_F: desc = F; break;
        case DESC_I: desc = I; break;
        case DESC_J: desc = J; break;
        case DESC_S: desc = S; break;
        case DESC_Z: desc = Z; break;
        case DESC_V: desc = V; break;
        }
    } else {
        desc = readString(p);
    }

    void* attributes = NULL;
    if (flags & MI_ATTRIBUTES) attributes = readPtr(p);

    void* impl = NULL;
    jint size = 0;
    void* synchronizedImpl = NULL;
    if (!IS_ABSTRACT(access)) {
        impl = readPtr(p);
        size = readInt(p);
        if (IS_SYNCHRONIZED(access)) synchronizedImpl = readPtr(p);
    }
    void** targetFnPtr = NULL;
    if (flags & MI_BRO_BRIDGE) targetFnPtr = readPtr(p);
    void* callbackImpl = NULL;
    if (flags & MI_BRO_CALLBACK) callbackImpl = readPtr(p);

    if (!callbacks->methodCallback) return TRUE;
    return callbacks->methodCallback(env, header, name, desc, access, size, impl, synchronizedImpl, targetFnPtr, callbackImpl, attributes, data);
}

void parseClassInfo(Env* env, ClassInfoHeader* header, ParseClassInfoCallbacks* callbacks, void* data) {
    jint flags = header->flags;

    jint access = 0;
    if (flags & CI_PUBLIC) access |= ACC_PUBLIC;
    if (flags & CI_FINAL) access |= ACC_FINAL;
    if (flags & CI_INTERFACE) access |= ACC_INTERFACE;
    if (flags & CI_ABSTRACT) access |= ACC_ABSTRACT;
    if (flags & CI_SYNTHETIC) access |= ACC_SYNTHETIC;
    if (flags & CI_ANNOTATION) access |= ACC_ANNOTATION;
    if (flags & CI_ENUM) access |= ACC_ENUM;

    flags >>= CI_FLAGS_BITS;
    jint methodCount = flags & CI_METHOD_COUNT_MASK;
    flags >>= CI_METHOD_COUNT_BITS;
    jint fieldCount = flags & CI_FIELD_COUNT_MASK;
    flags >>= CI_FIELD_COUNT_BITS;
    jint interfaceCount = flags & CI_INTERFACE_COUNT_MASK;

    void* p = ((void*) header) + sizeof(ClassInfoHeader);

    if (interfaceCount == CI_INTERFACE_COUNT_MASK) {
        interfaceCount = readShort(&p);
    }
    if (fieldCount == CI_FIELD_COUNT_MASK) {
        fieldCount = readShort(&p);
    }
    if (methodCount == CI_METHOD_COUNT_MASK) {
        methodCount = readShort(&p);
    }

    char* superclassName = NULL;
    if (!IS_INTERFACE(access)) {
        superclassName = readString(&p);
    }

    void* attributes = NULL;
    if (header->flags & CI_ATTRIBUTES) {
        attributes = readPtr(&p);
    }

    if (callbacks->classCallback) {
        if (!callbacks->classCallback(env, header, header->className, superclassName, 
                access, header->classDataSize, header->instanceDataSize, header->instanceDataOffset,
                attributes, header->initializer, data)) {
            return;
        }
    }

    if (!callbacks->interfaceCallback && !callbacks->fieldCallback && !callbacks->methodCallback) return;

    jint i;
    for (i = 0; i < interfaceCount; i++) {
        char* interfaceName = readString(&p);
        if (callbacks->interfaceCallback) {
            if (!callbacks->interfaceCallback(env, header, interfaceName, data)) return;
        }
    }

    if (!callbacks->fieldCallback && !callbacks->methodCallback) return;

    for (i = 0; i < fieldCount; i++) {
        if (!parseFieldInfo(env, header, &p, callbacks, data)) return;
    }

    if (!callbacks->methodCallback) return;

    for (i = 0; i < methodCount; i++) {
        if (!parseMethodInfo(env, header, &p, callbacks, data)) return;
    }
}

