#include "include/b3cpp_contact.h"
#include "include/b3cpp_types.h"

#include "private/b3cpp_utils.h"
#include "box3d/include/box3d/box3d.h" // Box3D types used internally

#include <cstdint>

namespace b3cpp
{
	Contact::Contact()
	{

	};

	bool Contact::isIdValid()
	{
		return b3Contact_IsValid(util::getId(this));
	}
}