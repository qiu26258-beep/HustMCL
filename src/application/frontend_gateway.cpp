#include "hustmcl/application/frontend_gateway.h"

#include <QMetaType>

namespace hustmcl::application {

FrontendGateway::FrontendGateway(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<HomeSnapshot>("hustmcl::application::HomeSnapshot");
    qRegisterMetaType<InstallSnapshot>("hustmcl::application::InstallSnapshot");
    qRegisterMetaType<TaskSnapshot>("hustmcl::application::TaskSnapshot");
    qRegisterMetaType<TaskError>("hustmcl::application::TaskError");
}

FrontendGateway::~FrontendGateway() = default;

} // namespace hustmcl::application
