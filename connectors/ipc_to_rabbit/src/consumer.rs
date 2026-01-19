use lapin::Channel;
use lapin::options::*;
use lapin::BasicProperties;

pub trait Consumer {
    async fn accept(&self, buf: Vec<u8>) -> Result<(), Box<dyn std::error::Error>>;
}

pub struct ConsumerImpl {
    channel: Channel,
    queue_name: String
}

impl ConsumerImpl {
    pub fn new(channel: Channel, queue_name: String) -> Self {
        Self {channel, queue_name }
    }
}

impl Consumer for ConsumerImpl {
    async fn accept(&self, buf: Vec<u8>) -> Result<(), Box<dyn std::error::Error>> {
        self.channel
            .basic_publish(
                "",                      // Default exchange (empty means "amq.direct")
                &self.queue_name,              // Routing key is the queue name
                BasicPublishOptions::default(),
                &buf,      // Message body
                BasicProperties::default(),
            )
            .await?
            .await?;

        println!("Message sent to RabbitMQ!");
        Ok(())
    }
}