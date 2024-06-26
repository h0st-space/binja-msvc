#include <binaryninjaapi.h>

#include "object_locator.h"
#include "utils.h"

using namespace BinaryNinja;

TypeDescriptor::TypeDescriptor(BinaryView* view, uint64_t address)
{
	BinaryReader reader = BinaryReader(view);
	reader.Seek(address);

	m_view = view;
	m_address = address;
	m_pVFTableValue = reader.ReadPointer();
	m_spareValue = reader.ReadPointer();
	m_nameValue = reader.ReadCString(512);
}

std::string TypeDescriptor::GetDemangledName()
{
	QualifiedName outName = {};
	Ref<Type> outTy = {};

	try
	{
		if (DemangleMS(m_view->GetDefaultArchitecture(), m_nameValue, outTy, outName, true))
		{
			return outName.GetString();
		}
	}
	catch (const std::exception& e)
	{
		LogError("Failed to demangle type descriptor %llx...", m_address);
	}

	return m_nameValue;
}

Ref<Type> TypeDescriptor::GetType()
{
	size_t addrSize = m_view->GetAddressSize();

	StructureBuilder typeDescriptorBuilder;
	typeDescriptorBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType(), true), "pVFTable");
	typeDescriptorBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "spare");
	// Char array needs to be individually resized.
	typeDescriptorBuilder.AddMember(Type::ArrayType(Type::IntegerType(1, true, "char"), m_nameValue.length()), "name");

	return TypeBuilder::StructureType(&typeDescriptorBuilder).Finalize();
}

Ref<Symbol> TypeDescriptor::CreateSymbol()
{
	Ref<Symbol> typeDescSym = new Symbol {DataSymbol, GetSymbolName(), m_address};
	m_view->DefineUserSymbol(typeDescSym);
	m_view->DefineUserDataVariable(m_address, GetType());
	return typeDescSym;
}

// Example: class Animal `RTTI Type Descriptor'
std::string TypeDescriptor::GetSymbolName()
{
	return "class " + GetDemangledName() + " `RTTI Type Descriptor'";
}