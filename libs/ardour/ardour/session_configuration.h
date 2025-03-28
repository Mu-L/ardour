/*
 * Copyright (C) 2009-2011 David Robillard <d@drobilla.net>
 * Copyright (C) 2009-2014 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2009 Carl Hetherington <carl@carlh.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include <map>
#include <string>

#include "ardour/types.h"
#include "pbd/configuration.h"

namespace ARDOUR {

class LIBARDOUR_API SessionConfiguration : public PBD::Configuration
{
public:
	SessionConfiguration ();

	void map_parameters (std::function<void (std::string)>&);
	int set_state (XMLNode const &, int version);
	XMLNode& get_state () const;
	XMLNode& get_variables (std::string const & nodename) const;
	void set_variables (XMLNode const &);

	bool load_state ();
	bool save_state ();

	/* define accessor methods */

	/* clang-format off */
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
#define CONFIG_VARIABLE(Type,var,name,value) \
	Type get_##var () const { return var.get(); } \
	bool set_##var (Type val) { bool ret = var.set (val); if (ret) { ParameterChanged (name); } return ret;  }
#define CONFIG_VARIABLE_SPECIAL(Type,var,name,value,mutator) \
	Type get_##var () const { return var.get(); } \
	bool set_##var (Type val) { bool ret = var.set (val); if (ret) { ParameterChanged (name); } return ret; }
#include "ardour/session_configuration_vars.inc.h"
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
	/* clang-format on */

  private:

	/* declare variables */

	/* clang-format off */
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
#define CONFIG_VARIABLE(Type,var,name,value) PBD::ConfigVariable<Type> var;
#define CONFIG_VARIABLE_SPECIAL(Type,var,name,value,mutator) PBD::ConfigVariableWithMutation<Type> var;
#include "ardour/session_configuration_vars.inc.h"
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
	/* clang-format on */

	int foo;

	std::map<std::string,PBD::ConfigVariableBase*> _my_variables;
};

}

