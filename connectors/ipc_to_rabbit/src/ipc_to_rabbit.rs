use crate::consumer::Consumer;
use crate::producer::Producer;

pub struct IpcToRabbit<C, P>
where C: Consumer,
    P: Producer
{
    consumer: C,
    producer: P
}

impl<C, P> IpcToRabbit<C, P>
where
    C: Consumer,
    P: Producer
{
    pub fn new(consumer: C, producer: P) -> Self {
        Self {consumer, producer}
    }

    pub async fn process(&self) {
        loop {
            match self.consumer.accept(self.producer.get()).await {
                Ok(_) => println!("Message processed successfully!"),
                Err(e) => eprintln!("Error processing message: {}!", e)
            }
        }
    }
}