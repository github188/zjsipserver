#include "b2bua/megaeyes/MegaSipEntity.hxx"

using namespace entity;

__gnu_cxx::hash_map<resip::Data, Terminal*> entity::Terminals;

Terminal* entity::getTerminal( const resip::Data& id )
{
    if ( entity::Terminals.find(id)!= entity::Terminals.end() )
    {
	return entity::Terminals[id];
    }
    else
    {
	return NULL;
    }
}
