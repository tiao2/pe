#!/bin/sh
exec "$JAVA_HOME/bin/java" -cp "gradle/wrapper/gradle-wrapper.jar" org.gradle.wrapper.GradleWrapperMain "$@"
