/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeKey.h>
#include <ply-reflect/SavedTypedPtr.h>
#include <ply-reflect/TypeSynthesizer.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/PersistWrite.h>

namespace ply {

SLOG_DECLARE_CHANNEL(Load)

TypeKey TypeKey_SavedTypedPtr{
    // write
    [](TypedPtr obj, WriteObjectContext* context) {
        SavedTypedPtr* savedTypedPtr = (SavedTypedPtr*) obj.ptr;
        TypeDescriptor* targetType = savedTypedPtr->owned.type;

        // Make sure the target's TypeDescriptor is written as part of the schema.
        // Assign and write its formatID.
        u32 targetFormatID = context->writeFormatContext->addOrGetFormatID(targetType);
        context->out.write<u32>(targetFormatID);

        // Write the target
        targetType->typeKey->write(savedTypedPtr->owned, context);
    },

    // writeFormat
    [](TypeDescriptor*, WriteFormatContext* context) { //
        context->writePrimitive(FormatKey::Typed);
    },

    // read
    [](TypedPtr obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        if ((FormatKey) formatDesc->formatKey != FormatKey::Typed) {
            SLOG(Load, "Can't load SavedTypedPtr");
            skip(context, formatDesc);
            return;
        }

        // Read formatID and look up the corresponding FormatDescriptor of the target.
        u32 targetFormatID = context->in.read<u32>();
        FormatDescriptor* targetFormat = context->schema->getFormatDesc(targetFormatID);

        // FIXME (just an idea): Could optionally find built-in reflected types instead of
        // always synthesizing one TypeDescriptor* targetType =
        // context->typeResolver->getType(targetFormat);

        // Synthesize TypeDescriptor with all its child types, and give the whole group a
        // TypeDescriptorOwner.
        Reference<TypeDescriptorOwner> targetTypeOwner = synthesizeType(targetFormat);
        TypeDescriptor* targetType = targetTypeOwner->getRootType();

        // Read the target
        SavedTypedPtr* savedTypedPtr = (SavedTypedPtr*) obj.ptr;
        savedTypedPtr->owned = TypedPtr::create(targetType);
        savedTypedPtr->typeOwner = targetTypeOwner;
        targetType->typeKey->read(savedTypedPtr->owned, context, targetFormat);
    },

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

PLY_NO_INLINE TypeDescriptor* TypeResolver<SavedTypedPtr>::get() {
    static TypeDescriptor typeDesc{&TypeKey_SavedTypedPtr, sizeof(SavedTypedPtr),
                                   NativeBindings::make<SavedTypedPtr>()};
    return &typeDesc;
}

} // namespace ply
