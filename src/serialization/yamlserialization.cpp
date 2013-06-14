#include "yamlserialization.h"

#include <yaml-cpp/yaml.h>

#include <QtCore/QFile>

#include <QtCore/QHashIterator>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include "../limbs/task.h"

YamlSerialization::YamlSerialization()
{
}

QByteArray YamlSerialization::serialize(Task *root)
{
	YAML::Emitter out;

	out << YAML::BeginMap;

	if ( !root->subtasks().empty() )
	{
		out << YAML::Key << "tasks";
		out << YAML::Value << YAML::BeginSeq;

		foreach(Task *subTask, root->subtasks())
			serializeTask(out, subTask);

		out << YAML::EndSeq;
	}

	out << YAML::EndMap;

	return QByteArray(out.c_str());
}

bool YamlSerialization::serialize(const QString &fileName, Task *root)
{
	QFile file(fileName);

	if ( file.open(QIODevice::WriteOnly|QIODevice::Text) )
	{
		file.write(YamlSerialization::serialize(root));
		file.flush();
		file.close();
		return true;
	}
	else
		return false;
}

void YamlSerialization::deserialize(const QByteArray &yaml, Task *root)
{
	YAML::Node node = YAML::Load(yaml.constData());

	deserializeRoot(node, root);
}

bool YamlSerialization::deserialize(const QString &fileName, Task *root)
{
	QFile file(fileName);

	if ( !file.exists() )
		return false;

	YAML::Node node = YAML::LoadFile(fileName.toStdString());

	deserializeRoot(node, root);

	return true;
}

QByteArray YamlSerialization::serializeSettings(const QVariantHash &settings)
{
	YAML::Emitter out;
	QStringList list;

	out << YAML::BeginMap;

	QHashIterator<QString, QVariant> i(settings);
	while ( i.hasNext() )
	{
		i.next();

		out << YAML::Key << i.key().toStdString();

		QVariant data = i.value();

		switch ( data.type() )
		{
			case QMetaType::QString:
				out << YAML::Value << data.toString().toStdString();
				break;

			case QMetaType::QByteArray:
				out << YAML::Value << QString(i.value().toByteArray().toHex()).toStdString();
				break;

			case QMetaType::QStringList:
				list = data.toStringList();
				out << YAML::Value << YAML::BeginSeq;

				foreach(const QString &string, list)
					out << YAML::Value << string.toStdString();

				out << YAML::EndSeq;
				break;

			default:
				out << YAML::Value << QString("(wrong type %1)").arg(i.value().type()).toStdString();
				break;
		}
	}

	out << YAML::EndMap;

	return QByteArray(out.c_str());
}

bool YamlSerialization::serializeSettings(const QString &fileName, const QVariantHash &settings)
{
	QFile file(fileName);

	if ( file.open(QIODevice::WriteOnly|QIODevice::Text) )
	{
		file.write(YamlSerialization::serializeSettings(settings));
		file.flush();
		file.close();
		return true;
	}
	else
		return false;
}

void YamlSerialization::deserializeSettings(const QByteArray &yaml, QVariantHash &settings)
{
	YAML::Node node = YAML::Load(yaml.constData());

	deserializeSettingsToHash(node, settings);
}

bool YamlSerialization::deserializeSettings(const QString &fileName, QVariantHash &settings)
{
	QFile file(fileName);

	if ( !file.exists() )
		return false;

	YAML::Node node = YAML::LoadFile(fileName.toStdString());

	deserializeSettingsToHash(node, settings);

	return true;
}

void YamlSerialization::serializeTask(YAML::Emitter &out, Task *task)
{
	out << YAML::BeginMap;

	out << YAML::Key << "description";
	out << YAML::Value << task->description().toStdString();

	out << YAML::Key << "done";
	out << YAML::Value << task->isDone();

	out << YAML::Key << "expanded";
	out << YAML::Value << task->isExpanded();

	if ( !task->subtasks().empty() )
	{
		out << YAML::Key << "tasks";
		out << YAML::Value << YAML::BeginSeq;

		foreach(Task *subTask, task->subtasks())
			serializeTask(out, subTask);

		out << YAML::EndSeq;
	}

	out << YAML::EndMap;
}

void YamlSerialization::deserializeRoot(const YAML::Node &node, Task *root)
{
	if ( node.IsNull() )
		return;

	if ( !node.IsMap() )
		return;

	if ( !node["tasks"] )
		return;

	YAML::Node tasks = node["tasks"];

	if ( tasks.IsNull() )
		return;

	if ( !tasks.IsSequence() )
		return;

	foreach(YAML::Node node, tasks)
		root->appendSubtask(deserializeTask(node));
}

Task *YamlSerialization::deserializeTask(const YAML::Node &node)
{
	if ( node.IsNull() )
		return 0;

	if ( !node.IsMap() )
		return 0;

	Task *task = new Task();

	if ( node["description"] )
		task->setDescription( QString::fromStdString(node["description"].as<std::string>()) );

	if ( node["done"] )
		task->setDone( node["done"].as<bool>() );

	if ( node["expanded"] )
		task->setExpanded( node["expanded"].as<bool>() );

	if ( !node["tasks"] )
		return task;

	YAML::Node tasks = node["tasks"];

	if ( tasks.IsNull() )
		return task;

	if ( !tasks.IsSequence() )
		return task;

	foreach(YAML::Node subTask, tasks)
		task->appendSubtask(deserializeTask(subTask));

	return task;
}

void YamlSerialization::deserializeSettingsToHash(const YAML::Node &node, QVariantHash &settings)
{
	if ( node.IsNull() )
		return;

	if ( !node.IsMap() )
		return;

	for(YAML::const_iterator it = node.begin(); it != node.end(); ++it)
	{
		if ( it->second.IsNull() )
			continue;

		if ( it->second.IsScalar() )
			settings[QString::fromStdString(it->first.as<std::string>())] = QString::fromStdString(it->second.as<std::string>());
		else if ( it->second.IsSequence() )
		{
			QStringList list;
			foreach(const YAML::Node &seq, it->second)
				list << QString::fromStdString(seq.as<std::string>());
			settings[QString::fromStdString(it->first.as<std::string>())] = list;
		}
	}
}
